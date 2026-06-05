#include "pch.h"

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#include "ConfigDialog.h"
#include "..\MatrixRainCore\AdapterSelection.h"
#include "..\MatrixRainCore\Application.h"
#include "..\MatrixRainCore\ApplicationState.h"
#include "..\MatrixRainCore\ConfigDialogController.h"
#include "..\MatrixRainCore\CommandLine.h"
#include "..\MatrixRainCore\WindowsAdapterProvider.h"
#include "resource.h"





struct DialogContext
{
    std::unique_ptr<ConfigDialogController>   m_controller;
    RegistrySettingsProvider                  m_settingsProvider;
    Application                             * m_pApp              = nullptr;
    bool                                      m_ownsContextMemory = false;
    bool                                      m_isScreenSaverCPL  = false;

    // Parallel to IDC_GPU_COMBO entries: each index holds the underlying
    // DXGI adapter description so OnGpuChange can map a selection back to
    // the persistence string.  The OS default adapter is annotated with
    // " (default)" in its display label; no synthetic <system default>
    // entry is added (the user picks the default adapter by name directly).
    std::vector<std::wstring>                 m_gpuAdapterDescriptions;

    // T058/T059 - shared tooltip control + the keyboard-activation TTF_TRACK
    // tool whose text we update on each info-button BN_CLICKED.
    HWND                                      m_hTooltip            = nullptr;

    // Larger font used to render the info-tip ⓘ glyph at 1.5x the dialog's
    // default font size in the BS_OWNERDRAW button paint path.  Created in
    // OnInitDialog from the dialog's WM_GETFONT; destroyed in OnDestroy.
    HFONT                                     m_hInfoTipFont        = nullptr;
};


// Sentinel uId for the single TTF_TRACK tool used by keyboard activation.
static constexpr UINT_PTR kTrackTipUId = 0xC0C00C0Cu;

// Subclass id for the info-button mouse-event relay (TTF_SUBCLASS proved
// unreliable on BS_OWNERDRAW buttons; we explicitly forward mouse events
// to the tooltip via TTM_RELAYEVENT instead).
static constexpr UINT_PTR kInfoButtonSubclassId = 0xC0C00C1Bu;




static LRESULT CALLBACK InfoButtonSubclassProc (HWND     hWnd,
                                                 UINT     msg,
                                                 WPARAM   wParam,
                                                 LPARAM   lParam,
                                                 UINT_PTR uIdSubclass,
                                                 DWORD_PTR dwRefData)
{
    HWND hTooltip = reinterpret_cast<HWND> (dwRefData);


    if (hTooltip)
    {
        switch (msg)
        {
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            {
                MSG ttMsg = {};
                ttMsg.hwnd    = hWnd;
                ttMsg.message = msg;
                ttMsg.wParam  = wParam;
                ttMsg.lParam  = lParam;
                SendMessageW (hTooltip, TTM_RELAYEVENT, 0, reinterpret_cast<LPARAM> (&ttMsg));
                break;
            }
        }
    }

    if (msg == WM_NCDESTROY)
    {
        RemoveWindowSubclass (hWnd, InfoButtonSubclassProc, uIdSubclass);
    }

    return DefSubclassProc (hWnd, msg, wParam, lParam);
}





static DialogContext * GetDialogContext (HWND hDlg)
{
    return reinterpret_cast<DialogContext *> (GetWindowLongPtr (hDlg, DWLP_USER));
}





static ConfigDialogController * GetControllerFromDialog (HWND hDlg)
{
    ConfigDialogController * pController = nullptr;
    DialogContext          * pContext    = GetDialogContext (hDlg);



    if (pContext)
    {
        pController = pContext->m_controller.get();
    }

    return pController;
}





static Application * GetApplicationFromDialog (HWND hDlg)
{
    Application   * pApp     = nullptr;
    DialogContext * pContext = GetDialogContext (hDlg);



    if (pContext)
    {
        pApp = pContext->m_pApp;
    }

    return pApp;
}





////////////////////////////////////////////////////////////////////////////////
//
//  GetInfoTipText — locked infotip strings (per the spec contract,
//  FR-036: descriptive sentence + standardized perf-impact sentence).
//
////////////////////////////////////////////////////////////////////////////////

static const wchar_t * GetInfoTipText (int infoId)
{
    switch (infoId)
    {
        case IDC_QUALITY_PRESET_INFO:
            return L"Picks a graphics quality preset. Higher presets look richer but "
                   L"use more GPU. Custom lets you tune the individual settings below.\r\n"
                   L"\r\n"
                   L"Significant GPU performance impact.";

        case IDC_GLOWINTENSITY_INFO:
            return L"Brightness of the glow effect around bright characters. Setting "
                   L"this to 0% disables the glow effect entirely.\r\n"
                   L"\r\n"
                   L"Significant GPU performance impact.";

        case IDC_GLOWSIZE_INFO:
            return L"Width of the glow halo around bright characters.";

        case IDC_GLOWPASSES_INFO:
            return L"How many times the glow is blurred. Each pass roughly doubles the "
                   L"glow's width.\r\n"
                   L"\r\n"
                   L"Significant GPU performance impact.";

        case IDC_GLOWRES_INFO:
            return L"Resolution the glow is computed at. Lower is much cheaper and only "
                   L"slightly softer; Quarter is about 4x cheaper than Full.\r\n"
                   L"\r\n"
                   L"Significant GPU performance impact.";

        case IDC_GLOWSMOOTH_INFO:
            return L"Number of samples per blur step. Higher gives smoother gradients "
                   L"with no banding.\r\n"
                   L"\r\n"
                   L"Moderate GPU performance impact.";

        default:
            return L"";
    }
}




static bool IsInfoTipControlId (int id)
{
    switch (id)
    {
        case IDC_QUALITY_PRESET_INFO:
        case IDC_GLOWINTENSITY_INFO:
        case IDC_GLOWSIZE_INFO:
        case IDC_GLOWPASSES_INFO:
        case IDC_GLOWRES_INFO:
        case IDC_GLOWSMOOTH_INFO:
            return true;
        default:
            return false;
    }
}




static int TickFrequencyForSliderId (int id)
{
    switch (id)
    {
        case IDC_DENSITY_SLIDER:        return 5;
        case IDC_ANIMSPEED_SLIDER:      return 5;
        case IDC_GLOWINTENSITY_SLIDER:  return 10;
        case IDC_GLOWSIZE_SLIDER:       return 10;  // 50..200 -> 16 ticks (midpoint at 125)
        case IDC_QUALITY_PRESET_SLIDER: return 1;
        case IDC_GLOWPASSES_SLIDER:     return 1;
        case IDC_GLOWRES_SLIDER:        return 1;
        case IDC_GLOWSMOOTH_SLIDER:     return 1;
        default:                        return 1;
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  CreateAndRegisterTooltip
//
//  Creates a shared WC_TOOLTIPS window for the config dialog and registers
//  every IDC_*_INFO button as a tool with TTF_IDISHWND | TTF_SUBCLASS, with
//  per-tool text supplied via TTN_GETDISPINFO (handled in ConfigDialogProc).
//
////////////////////////////////////////////////////////////////////////////////

static HWND CreateAndRegisterTooltip (HWND hDlg)
{
    static const int kInfoIds[] =
    {
        IDC_QUALITY_PRESET_INFO,
        IDC_GLOWINTENSITY_INFO,
        IDC_GLOWSIZE_INFO,
        IDC_GLOWPASSES_INFO,
        IDC_GLOWRES_INFO,
        IDC_GLOWSMOOTH_INFO,
    };


    HWND hTooltip = CreateWindowExW (WS_EX_TOPMOST,
                                     TOOLTIPS_CLASS,
                                     nullptr,
                                     WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     hDlg, nullptr, nullptr, nullptr);

    if (!hTooltip)
    {
        return nullptr;
    }

    SendMessageW (hTooltip, TTM_SETMAXTIPWIDTH, 0, 300);
    SendMessageW (hTooltip, TTM_ACTIVATE,       TRUE, 0);


    for (int infoId : kInfoIds)
    {
        HWND hCtrl = GetDlgItem (hDlg, infoId);

        if (!hCtrl)
        {
            continue;
        }

        TOOLINFOW ti = { sizeof (TOOLINFOW) };
        ti.uFlags    = TTF_IDISHWND;
        ti.hwnd      = hDlg;
        ti.uId       = (UINT_PTR) hCtrl;
        ti.lpszText  = LPSTR_TEXTCALLBACKW;

        SendMessageW (hTooltip, TTM_ADDTOOLW, 0, (LPARAM) &ti);

        // Manually subclass the button to forward mouse events to the
        // tooltip — TTF_SUBCLASS proved unreliable on BS_OWNERDRAW buttons.
        SetWindowSubclass (hCtrl,
                           InfoButtonSubclassProc,
                           kInfoButtonSubclassId,
                           reinterpret_cast<DWORD_PTR> (hTooltip));
    }


    // Single TTF_TRACK tool used for keyboard-activated tips (T059).  Its
    // text is updated on each info-button BN_CLICKED before we call
    // TTM_TRACKPOSITION + TTM_TRACKACTIVATE.
    TOOLINFOW trackTool = { sizeof (TOOLINFOW) };
    trackTool.uFlags    = TTF_TRACK | TTF_ABSOLUTE;
    trackTool.hwnd      = hDlg;
    trackTool.uId       = kTrackTipUId;
    trackTool.lpszText  = const_cast<LPWSTR> (L"");

    SendMessageW (hTooltip, TTM_ADDTOOLW, 0, (LPARAM) &trackTool);

    return hTooltip;
}




////////////////////////////////////////////////////////////////////////////////
//
//  OnInfoButtonClick — keyboard-activated tooltip for an IDC_*_INFO button
//  (T059).  Updates the shared TTF_TRACK tool's text to the matching infotip
//  string, positions it just below/right of the button, and activates it.
//  Auto-dismisses on a 5-second timer (handled in ConfigDialogProc).
//
////////////////////////////////////////////////////////////////////////////////

static constexpr UINT_PTR kInfoTipDismissTimerId = 0xC0C0C0DEu;

static void OnInfoButtonClick (HWND hDlg, int infoId)
{
    DialogContext * pContext = GetDialogContext (hDlg);

    if (!pContext || !pContext->m_hTooltip)
    {
        return;
    }

    HWND hButton = GetDlgItem (hDlg, infoId);

    if (!hButton)
    {
        return;
    }

    RECT btnRect;
    GetWindowRect (hButton, &btnRect);


    // Update the TTF_TRACK tool's text and position.
    TOOLINFOW ti = { sizeof (TOOLINFOW) };
    ti.hwnd      = hDlg;
    ti.uId       = kTrackTipUId;
    ti.lpszText  = const_cast<LPWSTR> (GetInfoTipText (infoId));

    SendMessageW (pContext->m_hTooltip, TTM_UPDATETIPTEXTW, 0, (LPARAM) &ti);

    SendMessageW (pContext->m_hTooltip,
                  TTM_TRACKPOSITION,
                  0,
                  MAKELPARAM (btnRect.right + 4, btnRect.bottom + 2));

    SendMessageW (pContext->m_hTooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM) &ti);

    SetTimer (hDlg, kInfoTipDismissTimerId, 5000, nullptr);
}




static void DismissInfoTip (HWND hDlg)
{
    DialogContext * pContext = GetDialogContext (hDlg);

    if (!pContext || !pContext->m_hTooltip)
    {
        return;
    }

    TOOLINFOW ti = { sizeof (TOOLINFOW) };
    ti.hwnd      = hDlg;
    ti.uId       = kTrackTipUId;

    SendMessageW (pContext->m_hTooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM) &ti);
    KillTimer (hDlg, kInfoTipDismissTimerId);
}




static std::wstring FormatPercentLabel (int sliderId, int value)
{
    // Glow Intensity reads "0% (glow disabled)" at 0 (FR-031).
    if (sliderId == IDC_GLOWINTENSITY_SLIDER && value == 0)
    {
        return std::wstring (L"0% (glow disabled)");
    }

    return std::format (L"{}%", value);
}




static const wchar_t * FormatResolutionLabel (int divisor)
{
    switch (divisor)
    {
        case 1: return L"Full";
        case 2: return L"Half";
        case 4: return L"Quarter";
        case 8: return L"Eighth";
        default: return L"Half";
    }
}




static const wchar_t * FormatSmoothnessLabel (int taps)
{
    switch (taps)
    {
        case 5:  return L"Low";
        case 9:  return L"Medium";
        case 13: return L"High";
        default: return L"High";
    }
}




static const wchar_t * FormatQualityPresetLabel (QualityPreset preset)
{
    switch (preset)
    {
        case QualityPreset::Low:    return L"Low";
        case QualityPreset::Medium: return L"Medium";
        case QualityPreset::High:   return L"High";
        case QualityPreset::Custom: return L"Custom";
        default:                    return L"Custom";
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  InitializeSlider
//
////////////////////////////////////////////////////////////////////////////////

static void InitializeSlider (HWND hDlg, int sliderId, int labelId, int minValue, int maxValue, int currentValue)
{
    SendDlgItemMessageW (hDlg, sliderId, TBM_SETRANGE,    TRUE, MAKELPARAM (minValue, maxValue));
    SendDlgItemMessageW (hDlg, sliderId, TBM_SETTICFREQ,  TickFrequencyForSliderId (sliderId), 0);

    // Speed (1..100) at freq=5 lands the last tick at 96; add an explicit
    // tick at 100 for the documented 21-tick total.
    if (sliderId == IDC_ANIMSPEED_SLIDER)
    {
        SendDlgItemMessageW (hDlg, sliderId, TBM_SETTIC, 0, 100);
    }

    SendDlgItemMessageW (hDlg, sliderId, TBM_SETPOS, TRUE, currentValue);
    SetDlgItemTextW     (hDlg, labelId, FormatPercentLabel (sliderId, currentValue).c_str());
}




////////////////////////////////////////////////////////////////////////////////
//
//  Discrete-slider initializers (Passes / Resolution / Smoothness).  These
//  use the same trackbar control but with small integer ranges and mapped
//  labels rather than percentages.
//
////////////////////////////////////////////////////////////////////////////////

static void InitializePassesSlider (HWND hDlg, int currentPasses)
{
    SendDlgItemMessageW (hDlg, IDC_GLOWPASSES_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM (1, 4));
    SendDlgItemMessageW (hDlg, IDC_GLOWPASSES_SLIDER, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessageW (hDlg, IDC_GLOWPASSES_SLIDER, TBM_SETPOS,   TRUE, currentPasses);
    SetDlgItemTextW     (hDlg, IDC_GLOWPASSES_LABEL, std::format (L"{}", currentPasses).c_str());
}


static void InitializeResolutionSlider (HWND hDlg, int currentDivisor)
{
    // Slider position 0..2 maps to divisor 4/2/1 (Quarter/Half/Full).
    // Eighth (divisor 8) is no longer offered through the UI; if a saved
    // configuration still has it, fall back to Quarter on display.
    int pos = 1;  // default Half

    switch (currentDivisor)
    {
        case 8: pos = 0; break;   // Eighth folds onto Quarter for display
        case 4: pos = 0; break;
        case 2: pos = 1; break;
        case 1: pos = 2; break;
    }

    SendDlgItemMessageW (hDlg, IDC_GLOWRES_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM (0, 2));
    SendDlgItemMessageW (hDlg, IDC_GLOWRES_SLIDER, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessageW (hDlg, IDC_GLOWRES_SLIDER, TBM_SETPOS,   TRUE, pos);
    SetDlgItemTextW     (hDlg, IDC_GLOWRES_LABEL, FormatResolutionLabel (currentDivisor));
}


static void InitializeSmoothnessSlider (HWND hDlg, int currentTaps)
{
    int pos = 2;  // default High

    switch (currentTaps)
    {
        case 5:  pos = 0; break;
        case 9:  pos = 1; break;
        case 13: pos = 2; break;
    }

    SendDlgItemMessageW (hDlg, IDC_GLOWSMOOTH_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM (0, 2));
    SendDlgItemMessageW (hDlg, IDC_GLOWSMOOTH_SLIDER, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessageW (hDlg, IDC_GLOWSMOOTH_SLIDER, TBM_SETPOS,   TRUE, pos);
    SetDlgItemTextW     (hDlg, IDC_GLOWSMOOTH_LABEL, FormatSmoothnessLabel (currentTaps));
}


static int ResolutionSliderPosToDivisor (int pos)
{
    switch (pos)
    {
        case 0: return 4;
        case 1: return 2;
        case 2: return 1;
        default: return 2;
    }
}


static int SmoothnessSliderPosToTaps (int pos)
{
    switch (pos)
    {
        case 0: return 5;
        case 1: return 9;
        case 2: return 13;
        default: return 13;
    }
}





// Color scheme combo entries: capitalized display label paired with the
// lowercase persisted key.  Index order MUST stay in sync between population,
// selection, and read-back so the UI label never leaks into storage/enums.
struct ColorSchemeEntry
{
    const WCHAR * key;
    const WCHAR * label;
};




static constexpr ColorSchemeEntry s_colorSchemeEntries[] =
{
    { L"green", L"Green" },
    { L"blue",  L"Blue"  },
    { L"red",   L"Red"   },
    { L"amber", L"Amber" },
    { L"cycle", L"Cycle" },
};




////////////////////////////////////////////////////////////////////////////////
//
//  ColorSchemeKeyToIndex
//
////////////////////////////////////////////////////////////////////////////////

static int ColorSchemeKeyToIndex (const std::wstring & key)
{
    for (int i = 0; i < static_cast<int> (ARRAYSIZE (s_colorSchemeEntries)); i++)
    {
        if (key == s_colorSchemeEntries[i].key)
            return i;
    }

    return 0;
}




////////////////////////////////////////////////////////////////////////////////
//
//  InitializeColorSchemeCombo
//
////////////////////////////////////////////////////////////////////////////////

static void InitializeColorSchemeCombo (HWND hDlg, const std::wstring & currentScheme)
{
    for (const ColorSchemeEntry & entry : s_colorSchemeEntries)
    {
        SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_ADDSTRING, 0, (LPARAM) entry.label);
    }

    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_SETCURSEL, ColorSchemeKeyToIndex (currentScheme), 0);
}





////////////////////////////////////////////////////////////////////////////////
//
//  InitializeGpuCombo
//
//  Enumerate the system's rendering adapters via WindowsAdapterProvider and
//  populate IDC_GPU_COMBO with their real names; the OS default adapter is
//  annotated with " (default)" via FormatAdapterLabel.  The user picks the
//  default adapter by name directly (no synthetic <system default> entry).
//
//  Selection logic:
//   - currentDescription matches an enumerated adapter -> select that entry.
//   - currentDescription is empty or doesn't match -> select the (default)
//     adapter so the UI still highlights what is actually running.
//
////////////////////////////////////////////////////////////////////////////////

static void InitializeGpuCombo (HWND hDlg, DialogContext * pContext, const std::wstring & currentDescription)
{
    WindowsAdapterProvider   provider;
    std::vector<AdapterInfo> adapters       = provider.EnumerateAdapters();
    int                      selected       = -1;
    int                      defaultIndex   = -1;


    pContext->m_gpuAdapterDescriptions.clear();

    for (const AdapterInfo & adapter : adapters)
    {
        std::wstring label = FormatAdapterLabel (adapter);

        SendDlgItemMessageW (hDlg, IDC_GPU_COMBO, CB_ADDSTRING, 0, (LPARAM) label.c_str());

        pContext->m_gpuAdapterDescriptions.push_back (adapter.m_description);

        int idx = static_cast<int> (pContext->m_gpuAdapterDescriptions.size()) - 1;

        if (adapter.m_isDefault)
        {
            defaultIndex = idx;
        }

        if (!currentDescription.empty() && adapter.m_description == currentDescription)
        {
            selected = idx;
        }
    }

    if (selected < 0)
    {
        selected = defaultIndex;
    }

    if (selected >= 0)
    {
        SendDlgItemMessageW (hDlg, IDC_GPU_COMBO, CB_SETCURSEL, selected, 0);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnInitDialog
//
////////////////////////////////////////////////////////////////////////////////

static BOOL OnInitDialog (HWND hDlg, LPARAM initParam)
{
    HRESULT                     hr           = S_OK;
    BOOL                        fSuccess     = FALSE;
    HWND                        parentHwnd   = GetParent (hDlg);
    RECT                        dialogRect   = {};
    RECT                        centerRect   = {};
    int                         dialogWidth  = 0;
    int                         dialogHeight = 0;
    int                         centerX      = 0;
    int                         centerY      = 0;
    POINT                       dialogPos    = {};
    DialogContext             * pContext     = reinterpret_cast<DialogContext *> (initParam);
    const ScreenSaverSettings * pSettings    = nullptr;
    WINDOWPLACEMENT             wp           = { sizeof (WINDOWPLACEMENT) };



    CBRAEx (pContext != nullptr && pContext->m_controller != nullptr, E_UNEXPECTED);

    SetWindowLongPtr (hDlg, DWLP_USER, reinterpret_cast<LONG_PTR> (pContext));

    // Center dialog for /c (no HWND) or live overlay modes
    // Skip centering for /c:<HWND> (Control Panel with parent)
    if (!parentHwnd || pContext->m_controller->IsLiveMode())
    {
        HWND appHwnd = nullptr;
        
        
        
        // Get window to center on (application window or primary monitor)
        GetWindowRect (hDlg, &dialogRect);
        dialogWidth  = dialogRect.right  - dialogRect.left;
        dialogHeight = dialogRect.bottom - dialogRect.top;

        if (pContext->m_pApp != nullptr)
        {
            // Live overlay mode - center on application window
            appHwnd = pContext->m_pApp->GetMainWindowHwnd();
        }
        
        if (appHwnd)
        {
            GetWindowRect (appHwnd, &centerRect);       
        }
        else
        {
            // No app window - center on primary monitor
            centerRect.left   = 0;
            centerRect.top    = 0;
            centerRect.right  = GetSystemMetrics (SM_CXSCREEN);
            centerRect.bottom = GetSystemMetrics (SM_CYSCREEN);
        }
        
        // Calculate centered position
        centerX = (centerRect.left + centerRect.right  - dialogWidth)  / 2;
        centerY = (centerRect.top  + centerRect.bottom - dialogHeight) / 2;
        
        dialogPos.x = centerX;
        dialogPos.y = centerY;
        
        SetWindowPos (hDlg, nullptr, dialogPos.x, dialogPos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    // else: Control Panel mode with parent HWND - let Windows handle positioning

#ifndef _DEBUG
    if (pContext->m_controller->IsLiveMode())
    {
        if (parentHwnd)
        {
            if (GetWindowPlacement (parentHwnd, &wp) && wp.showCmd == SW_SHOWMAXIMIZED)
            {
                SetWindowPos (hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            }
        }
    }
#endif

    pSettings = &pContext->m_controller->GetSettings();
    
    InitializeSlider (hDlg, IDC_DENSITY_SLIDER, IDC_DENSITY_LABEL, 
                      ScreenSaverSettings::MIN_DENSITY_PERCENT, ScreenSaverSettings::MAX_DENSITY_PERCENT, 
                      pSettings->m_densityPercent);
    
    InitializeSlider (hDlg, IDC_ANIMSPEED_SLIDER, IDC_ANIMSPEED_LABEL,
                      ScreenSaverSettings::MIN_ANIMATION_SPEED_PERCENT, ScreenSaverSettings::MAX_ANIMATION_SPEED_PERCENT,
                      pSettings->m_animationSpeedPercent);
    
    InitializeSlider (hDlg, IDC_GLOWINTENSITY_SLIDER, IDC_GLOWINTENSITY_LABEL,
                      ScreenSaverSettings::MIN_GLOW_INTENSITY_PERCENT, ScreenSaverSettings::MAX_GLOW_INTENSITY_PERCENT,
                      pSettings->m_glowIntensityPercent);
    
    InitializeSlider (hDlg, IDC_GLOWSIZE_SLIDER, IDC_GLOWSIZE_LABEL,
                      ScreenSaverSettings::MIN_GLOW_SIZE_PERCENT, ScreenSaverSettings::MAX_GLOW_SIZE_PERCENT,
                      pSettings->m_glowSizePercent);
    
    InitializeColorSchemeCombo (hDlg, pSettings->m_colorSchemeKey);
    InitializeGpuCombo         (hDlg, pContext, pSettings->m_gpuAdapter);

    // Quality preset combo + advanced disclosure.  Three named presets +
    // Custom; the dialog code selects whichever matches the loaded
    // settings.
    // Quality preset slider (0=Low, 1=Medium, 2=High, 3=Custom).  Replaces
     // the v1.4 combobox per UX feedback — discrete trackbar with named
     // value label matches the rest of the dialog's slider+label pattern.
    SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETRANGE,   TRUE, MAKELPARAM (0, 3));
    SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS,     TRUE, static_cast<int> (pSettings->m_qualityPreset));
    SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (pSettings->m_qualityPreset));

    InitializePassesSlider      (hDlg, pSettings->m_advancedValues.m_blurPasses);
    InitializeResolutionSlider  (hDlg, static_cast<int> (pSettings->m_advancedValues.m_bloomResolutionDivisor));
    InitializeSmoothnessSlider  (hDlg, static_cast<int> (pSettings->m_advancedValues.m_blurTaps));

    // Tooltip surface for the IDC_*_INFO indicators (FR-034/FR-035/FR-036).
    // The TTN_GETDISPINFO notification is handled in ConfigDialogProc.
    pContext->m_hTooltip = CreateAndRegisterTooltip (hDlg);

    // Create the 1.5x-size font used by the owner-drawn ⓘ glyphs.  The
    // base font comes from the dialog itself (WM_GETFONT).
    {
        HFONT hDialogFont = reinterpret_cast<HFONT> (SendMessageW (hDlg, WM_GETFONT, 0, 0));
        LOGFONTW lf       = {};

        if (hDialogFont && GetObjectW (hDialogFont, sizeof (lf), &lf))
        {
            // lfHeight is negative for character-cell heights; multiply
            // absolute value by 1.5 and preserve sign.
            lf.lfHeight = (lf.lfHeight < 0)
                            ? -static_cast<LONG> ((-lf.lfHeight) * 3 / 2)
                            :  static_cast<LONG> ( lf.lfHeight * 3 / 2);

            pContext->m_hInfoTipFont = CreateFontIndirectW (&lf);
        }
    }
    
    CheckDlgButton (hDlg, IDC_STARTFULLSCREEN_CHECK, pSettings->m_startFullscreen     ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_MULTIMONITOR_CHECK,    pSettings->m_multiMonitorEnabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWDEBUG_CHECK,       pSettings->m_showDebugStats      ? BST_CHECKED : BST_UNCHECKED);

    // Hide fullscreen checkbox in screensaver CPL mode — screensaver always forces fullscreen
    if (pContext->m_isScreenSaverCPL)
    {
        ShowWindow (GetDlgItem (hDlg, IDC_STARTFULLSCREEN_CHECK), SW_HIDE);
    }
#ifdef _DEBUG
    CheckDlgButton (hDlg, IDC_SHOWFADETIMERS_CHECK,  pSettings->m_showFadeTimers  ? BST_CHECKED : BST_UNCHECKED);
#else
    ShowWindow (GetDlgItem (hDlg, IDC_SHOWFADETIMERS_CHECK), SW_HIDE);
#endif
    
    fSuccess = TRUE;

Error:
    return fSuccess;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnHScroll
//
////////////////////////////////////////////////////////////////////////////////

static void OnQualityPresetChange (HWND hDlg);

static BOOL OnHScroll (HWND hDlg, LPARAM lParam)
{
    HRESULT                  hr          = S_OK;
    BOOL                     fSuccess    = FALSE;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);
    HWND                     hSlider     = (HWND)lParam;
    int                      ctrlId      = 0;
    int                      pos         = 0;



    CBRAEx (pController != nullptr, E_UNEXPECTED);
    CBRAEx (hSlider     != nullptr, E_UNEXPECTED);

    ctrlId = GetDlgCtrlID (hSlider);
    pos    = (int)SendMessageW (hSlider, TBM_GETPOS, 0, 0);
    
    switch (ctrlId)
    {
        case IDC_DENSITY_SLIDER:
            pController->UpdateDensity (pos);
            SetDlgItemTextW (hDlg, IDC_DENSITY_LABEL, FormatPercentLabel (ctrlId, pos).c_str());
            break;
            
        case IDC_ANIMSPEED_SLIDER:
            pController->UpdateAnimationSpeed (pos);
            SetDlgItemTextW (hDlg, IDC_ANIMSPEED_LABEL, FormatPercentLabel (ctrlId, pos).c_str());
            break;
            
        case IDC_GLOWINTENSITY_SLIDER:
        {
            pController->UpdateGlowIntensity (pos);
            SetDlgItemTextW (hDlg, IDC_GLOWINTENSITY_LABEL, FormatPercentLabel (ctrlId, pos).c_str());

            // Glow Intensity is part of the advanced-graphics value set;
            // changing it drifts the preset to Custom (FR-023) the same
            // way moving Passes / Resolution / Smoothness would.
            AdvancedGraphicsValues v = pController->GetSettings().m_advancedValues;
            v.m_glowIntensityPercent = pos;
            pController->UpdateAdvancedGraphicsValues (v);
            {
                QualityPreset p = pController->GetSettings().m_qualityPreset;
                SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS, TRUE, static_cast<int> (p));
                SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (p));
            }
            break;
        }
            
        case IDC_GLOWSIZE_SLIDER:
            pController->UpdateGlowSize (pos);
            SetDlgItemTextW (hDlg, IDC_GLOWSIZE_LABEL, FormatPercentLabel (ctrlId, pos).c_str());
            break;

        case IDC_GLOWPASSES_SLIDER:
        {
            AdvancedGraphicsValues v = pController->GetSettings().m_advancedValues;
            v.m_blurPasses = pos;
            pController->UpdateAdvancedGraphicsValues (v);
            SetDlgItemTextW (hDlg, IDC_GLOWPASSES_LABEL, std::format (L"{}", pos).c_str());
            {
                QualityPreset p = pController->GetSettings().m_qualityPreset;
                SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS, TRUE, static_cast<int> (p));
                SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (p));
            }
            break;
        }

        case IDC_GLOWRES_SLIDER:
        {
            int divisor = ResolutionSliderPosToDivisor (pos);
            AdvancedGraphicsValues v = pController->GetSettings().m_advancedValues;
            v.m_bloomResolutionDivisor = static_cast<ResolutionDivisor> (divisor);
            pController->UpdateAdvancedGraphicsValues (v);
            SetDlgItemTextW (hDlg, IDC_GLOWRES_LABEL, FormatResolutionLabel (divisor));
            {
                QualityPreset p = pController->GetSettings().m_qualityPreset;
                SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS, TRUE, static_cast<int> (p));
                SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (p));
            }
            break;
        }

        case IDC_GLOWSMOOTH_SLIDER:
        {
            int taps = SmoothnessSliderPosToTaps (pos);
            AdvancedGraphicsValues v = pController->GetSettings().m_advancedValues;
            v.m_blurTaps = static_cast<BlurTaps> (taps);
            pController->UpdateAdvancedGraphicsValues (v);
            SetDlgItemTextW (hDlg, IDC_GLOWSMOOTH_LABEL, FormatSmoothnessLabel (taps));
            {
                QualityPreset p = pController->GetSettings().m_qualityPreset;
                SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS, TRUE, static_cast<int> (p));
                SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (p));
            }
            break;
        }

        case IDC_QUALITY_PRESET_SLIDER:
            OnQualityPresetChange (hDlg);
            break;
    }
    
    fSuccess = TRUE;

Error:
    return fSuccess;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnColorSchemeChange
//
////////////////////////////////////////////////////////////////////////////////

static void OnColorSchemeChange (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);
    int                      index       = 0;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    index = (int) SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_GETCURSEL, 0, 0);

    CBRAEx (index >= 0 && index < static_cast<int> (ARRAYSIZE (s_colorSchemeEntries)), E_UNEXPECTED);

    pController->UpdateColorScheme (s_colorSchemeEntries[index].key);

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnGpuChange
//
////////////////////////////////////////////////////////////////////////////////

static void OnGpuChange (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    DialogContext          * pContext    = GetDialogContext (hDlg);
    ConfigDialogController * pController = nullptr;
    int                      index       = 0;



    CBRAEx (pContext != nullptr && pContext->m_controller != nullptr, E_UNEXPECTED);

    pController = pContext->m_controller.get();

    index = (int) SendDlgItemMessageW (hDlg, IDC_GPU_COMBO, CB_GETCURSEL, 0, 0);

    CBRAEx (index >= 0 && index < static_cast<int> (pContext->m_gpuAdapterDescriptions.size()), E_UNEXPECTED);

    pController->UpdateGpuAdapter (pContext->m_gpuAdapterDescriptions[index]);

    // Live preview: post a rebuild so the running app recreates its
    // contexts on the newly-chosen GPU within 1 second (FR-015).
    if (Application * pApp = GetApplicationFromDialog (hDlg))
    {
        pApp->ApplyDisplayModeChange();
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnQualityPresetChange
//
////////////////////////////////////////////////////////////////////////////////

static void OnQualityPresetChange (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);
    int                      index       = 0;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    index = (int) SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_GETPOS, 0, 0);

    CBRAEx (index >= 0 && index <= 3, E_UNEXPECTED);

    pController->UpdateQualityPreset (static_cast<QualityPreset> (index));

    {
        // Reflect the snapped advanced values back into the three sliders and
        // the Glow Intensity slider; the controller already updated them.
        const ScreenSaverSettings & s = pController->GetSettings();

        SetDlgItemTextW (hDlg, IDC_QUALITY_PRESET_LABEL, FormatQualityPresetLabel (s.m_qualityPreset));

        SendDlgItemMessageW (hDlg, IDC_GLOWINTENSITY_SLIDER, TBM_SETPOS, TRUE, s.m_advancedValues.m_glowIntensityPercent);
        SetDlgItemTextW     (hDlg, IDC_GLOWINTENSITY_LABEL,
                             FormatPercentLabel (IDC_GLOWINTENSITY_SLIDER, s.m_advancedValues.m_glowIntensityPercent).c_str());

        InitializePassesSlider     (hDlg, s.m_advancedValues.m_blurPasses);
        InitializeResolutionSlider (hDlg, static_cast<int> (s.m_advancedValues.m_bloomResolutionDivisor));
        InitializeSmoothnessSlider (hDlg, static_cast<int> (s.m_advancedValues.m_blurTaps));
    }

Error:
    return;
}




////////////////////////////////////////////////////////////////////////////////
//
//  OnStartFullscreenCheck
//
////////////////////////////////////////////////////////////////////////////////

static void OnStartFullscreenCheck (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);
    bool                     checked     = IsDlgButtonChecked (hDlg, IDC_STARTFULLSCREEN_CHECK) == BST_CHECKED;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    pController->UpdateStartFullscreen (checked);
    
    // If live overlay mode, immediately apply fullscreen/windowed state
    if (Application * pApp = GetApplicationFromDialog (hDlg))
    {
        DisplayMode newMode = checked ? DisplayMode::Fullscreen : DisplayMode::Windowed;
        pApp->GetApplicationState()->SetDisplayMode (newMode);
        pApp->ApplyDisplayModeChange();
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnMultiMonitorCheck
//
////////////////////////////////////////////////////////////////////////////////

static void OnMultiMonitorCheck (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);
    bool                     checked     = IsDlgButtonChecked (hDlg, IDC_MULTIMONITOR_CHECK) == BST_CHECKED;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    pController->UpdateMultiMonitorEnabled (checked);

    // Live preview: post a rebuild so the running app applies the new
    // multimon gate within the next message-loop tick.  Reuses the same
    // message handler the display-mode toggle relies on.
    if (Application * pApp = GetApplicationFromDialog (hDlg))
    {
        pApp->ApplyDisplayModeChange();
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnShowDebugCheck
//
////////////////////////////////////////////////////////////////////////////////

static void OnShowDebugCheck (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);
    bool                     checked     = IsDlgButtonChecked (hDlg, IDC_SHOWDEBUG_CHECK) == BST_CHECKED;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    pController->UpdateShowDebugStats (checked);
    
    // If live overlay mode, immediately apply debug stats display
    if (Application * pApp = GetApplicationFromDialog (hDlg))
    {
        pApp->GetApplicationState()->SetShowStatistics (checked);
    }

Error:
    return;
}





#ifdef _DEBUG
////////////////////////////////////////////////////////////////////////////////
//
//  OnShowFadeTimersCheck
//
////////////////////////////////////////////////////////////////////////////////

static void OnShowFadeTimersCheck (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);
    bool                     checked     = IsDlgButtonChecked (hDlg, IDC_SHOWFADETIMERS_CHECK) == BST_CHECKED;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    pController->UpdateShowFadeTimers (checked);
    
    // If live overlay mode, immediately apply fade timers display
    if (Application * pApp = GetApplicationFromDialog (hDlg))
    {
        pApp->GetApplicationState()->SetShowDebugFadeTimes (checked);
    }

Error:
    return;
}
#endif





////////////////////////////////////////////////////////////////////////////////
//
//  OnResetButton
//
////////////////////////////////////////////////////////////////////////////////

static void OnResetButton (HWND hDlg)
{
    HRESULT                     hr          = S_OK;
    ConfigDialogController    * pController = GetControllerFromDialog (hDlg);
    const ScreenSaverSettings * pDefaults   = nullptr;
    int                         schemeIndex = 0;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    pController->ResetToDefaults();
    
    pDefaults = &pController->GetSettings();
    
    // Update UI controls
    SendDlgItemMessageW (hDlg, IDC_DENSITY_SLIDER, TBM_SETPOS, TRUE, pDefaults->m_densityPercent);
    SetDlgItemTextW     (hDlg, IDC_DENSITY_LABEL, std::format (L"{}%", pDefaults->m_densityPercent).c_str());
    
    SendDlgItemMessageW (hDlg, IDC_ANIMSPEED_SLIDER, TBM_SETPOS, TRUE, pDefaults->m_animationSpeedPercent);
    SetDlgItemTextW     (hDlg, IDC_ANIMSPEED_LABEL, std::format (L"{}%", pDefaults->m_animationSpeedPercent).c_str());
    
    SendDlgItemMessageW (hDlg, IDC_GLOWINTENSITY_SLIDER, TBM_SETPOS, TRUE, pDefaults->m_glowIntensityPercent);
    SetDlgItemTextW     (hDlg, IDC_GLOWINTENSITY_LABEL, std::format (L"{}%", pDefaults->m_glowIntensityPercent).c_str());
    
    SendDlgItemMessageW (hDlg, IDC_GLOWSIZE_SLIDER, TBM_SETPOS, TRUE, pDefaults->m_glowSizePercent);
    SetDlgItemTextW     (hDlg, IDC_GLOWSIZE_LABEL, std::format (L"{}%", pDefaults->m_glowSizePercent).c_str());
    
    // Update color scheme combo box
    schemeIndex = ColorSchemeKeyToIndex (pDefaults->m_colorSchemeKey);
    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_SETCURSEL, schemeIndex, 0);

    // v1.4 additions — multimon, GPU, quality preset slider, advanced sliders.
    CheckDlgButton (hDlg, IDC_MULTIMONITOR_CHECK, pDefaults->m_multiMonitorEnabled ? BST_CHECKED : BST_UNCHECKED);

    {
        DialogContext * pCtx = GetDialogContext (hDlg);

        if (pCtx)
        {
            // Re-resolve the default-marked entry in the existing combo.
            int defaultIdx = 0;

            for (size_t i = 0; i < pCtx->m_gpuAdapterDescriptions.size(); i++)
            {
                if (pCtx->m_gpuAdapterDescriptions[i] == pDefaults->m_gpuAdapter)
                {
                    defaultIdx = static_cast<int> (i);
                    break;
                }
            }

            SendDlgItemMessageW (hDlg, IDC_GPU_COMBO, CB_SETCURSEL, defaultIdx, 0);
        }
    }

    SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS, TRUE, static_cast<int> (pDefaults->m_qualityPreset));
    SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (pDefaults->m_qualityPreset));

    InitializePassesSlider      (hDlg, pDefaults->m_advancedValues.m_blurPasses);
    InitializeResolutionSlider  (hDlg, static_cast<int> (pDefaults->m_advancedValues.m_bloomResolutionDivisor));
    InitializeSmoothnessSlider  (hDlg, static_cast<int> (pDefaults->m_advancedValues.m_blurTaps));

    CheckDlgButton (hDlg, IDC_STARTFULLSCREEN_CHECK, pDefaults->m_startFullscreen ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWDEBUG_CHECK,       pDefaults->m_showDebugStats  ? BST_CHECKED : BST_UNCHECKED);
#ifdef _DEBUG
    CheckDlgButton (hDlg, IDC_SHOWFADETIMERS_CHECK,  pDefaults->m_showFadeTimers  ? BST_CHECKED : BST_UNCHECKED);
#endif
    
    // If live overlay mode, propagate changes to running application
    // (ResetToDefaults already updated controller settings, now trigger live propagation)
    CBRAEx (pController != nullptr, E_UNEXPECTED);
    if (pController->IsLiveMode())
    {
        pController->UpdateDensity                (pDefaults->m_densityPercent);
        pController->UpdateAnimationSpeed         (pDefaults->m_animationSpeedPercent);
        pController->UpdateGlowIntensity          (pDefaults->m_glowIntensityPercent);
        pController->UpdateGlowSize               (pDefaults->m_glowSizePercent);
        pController->UpdateColorScheme            (pDefaults->m_colorSchemeKey.c_str());
        pController->UpdateAdvancedGraphicsValues (pDefaults->m_advancedValues);
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnOK
//
////////////////////////////////////////////////////////////////////////////////

static BOOL OnOK (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    BOOL                     fSuccess    = FALSE;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    hr = pController->ApplyChanges();
    CHRL (hr, L"Failed to save settings to registry");
    
    // For modeless dialogs, use DestroyWindow instead of EndDialog
    if (pController->IsLiveMode())
    {
        DestroyWindow (hDlg);
    }
    else
    {
        EndDialog (hDlg, IDOK);
    }
    
    fSuccess = TRUE;

Error:
    return fSuccess;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnCancel
//
////////////////////////////////////////////////////////////////////////////////

static BOOL OnCancel (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    BOOL                     fSuccess    = FALSE;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    // For modeless dialogs, revert live preview changes back to snapshot
    if (pController->IsLiveMode())
    {
        Application * pApp           = GetApplicationFromDialog (hDlg);
        bool          needsRebuild   = pController->LiveModeRebuildRequired();

        pController->CancelLiveMode();

        // Only rebuild contexts when a field that requires destroy/recreate
        // (multimon span, GPU adapter) actually changed — otherwise the
        // user sees an ugly flicker on every dialog close.
        if (pApp && needsRebuild)
        {
            pApp->ApplyDisplayModeChange();
        }

        // Clear the dialog handle before destroying so input handling resumes
        if (pApp)
        {
            pApp->SetConfigDialog (nullptr);
        }

        DestroyWindow (hDlg);
    }
    else
    {
        pController->CancelChanges();
        EndDialog (hDlg, IDCANCEL);
    }
    
    fSuccess = TRUE;

Error:
    return fSuccess;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnCommand
//
////////////////////////////////////////////////////////////////////////////////

static BOOL OnCommand (HWND hDlg, WPARAM wParam)
{
    HRESULT                  hr          = S_OK;
    BOOL                     fSuccess    = FALSE;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    switch (LOWORD (wParam))
    {
        case IDC_COLORSCHEME_COMBO:
            if (HIWORD (wParam) == CBN_SELCHANGE)
            {
                OnColorSchemeChange (hDlg);
            }
            break;

        case IDC_GPU_COMBO:
            if (HIWORD (wParam) == CBN_SELCHANGE)
            {
                OnGpuChange (hDlg);
            }
            break;

        case IDC_QUALITY_PRESET_INFO:
        case IDC_GLOWINTENSITY_INFO:
        case IDC_GLOWSIZE_INFO:
        case IDC_GLOWPASSES_INFO:
        case IDC_GLOWRES_INFO:
        case IDC_GLOWSMOOTH_INFO:
            // BN_CLICKED on any info button (mouse click OR Space/Enter on
            // a keyboard-focused button) pops the matching infotip via
            // TTM_TRACKACTIVATE.  Auto-dismisses after 5s.
            OnInfoButtonClick (hDlg, LOWORD (wParam));
            break;
            
        case IDC_STARTFULLSCREEN_CHECK:
            OnStartFullscreenCheck (hDlg);
            break;

        case IDC_MULTIMONITOR_CHECK:
            OnMultiMonitorCheck (hDlg);
            break;
            
        case IDC_SHOWDEBUG_CHECK:
            OnShowDebugCheck (hDlg);
            break;
            
#ifdef _DEBUG
        case IDC_SHOWFADETIMERS_CHECK:
            OnShowFadeTimersCheck (hDlg);
            break;
#endif
            
        case IDC_RESET_BUTTON:
            OnResetButton (hDlg);
            break;
            
        case IDOK:
            fSuccess = OnOK (hDlg);
            break;
            
        case IDCANCEL:
            fSuccess = OnCancel (hDlg);
            break;
    }

Error:
    return fSuccess;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnDestroy
//
////////////////////////////////////////////////////////////////////////////////

static void OnDestroy (HWND hDlg)
{
    DialogContext          * pContext    = GetDialogContext (hDlg);
    ConfigDialogController * pController = pContext ? pContext->m_controller.get() : nullptr;



    if (pController && pController->IsLiveMode())
    {
        Application * pApp = GetApplicationFromDialog (hDlg);



        if (pApp)
        {
            pApp->SetConfigDialog (nullptr);

            // Only quit the app if it was launched in /c settings-only mode
            if (pApp->GetScreenSaverMode() == ScreenSaverMode::SettingsDialog)
            {
                PostQuitMessage (0);
            }
        }
    }



    if (pContext)
    {
        SetWindowLongPtr (hDlg, DWLP_USER, 0);

        if (pContext->m_hInfoTipFont)
        {
            DeleteObject (pContext->m_hInfoTipFont);
            pContext->m_hInfoTipFont = nullptr;
        }
        
        if (pContext->m_ownsContextMemory)
        {
            delete pContext;
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogProc
//
//  Dialog procedure for IDD_MATRIXRAIN_SAVER_CONFIG.
//
////////////////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK ConfigDialogProc (HWND   hDlg,
                                           UINT   message,
                                           WPARAM wParam,
                                           LPARAM lParam)
{
    INT_PTR result = FALSE;



    switch (message)
    {
        case WM_INITDIALOG:
            result = OnInitDialog (hDlg, lParam);
            break;
        
        case WM_HSCROLL:
            result = OnHScroll (hDlg, lParam);
            break;
        
        case WM_COMMAND:
            result = OnCommand (hDlg, wParam);
            break;

        case WM_DRAWITEM:
        {
            // Owner-draw paint for the ⓘ info indicators.  No button frame:
            // we draw only the glyph (transparent background, 1.5x font).
            LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT> (lParam);

            if (pdis && pdis->CtlType == ODT_BUTTON && IsInfoTipControlId (pdis->CtlID))
            {
                DialogContext * pContext = GetDialogContext (hDlg);
                HFONT           hOldFont = nullptr;
                int             oldBkMode;
                COLORREF        oldTextColor;
                wchar_t         glyph[]  = L"\u24D8";

                // Erase the entire rect with the dialog's face color so any
                // previous focus rect (XOR-drawn by the system) is cleared
                // before we redraw the glyph + optional new focus rect.
                FillRect (pdis->hDC, &pdis->rcItem, GetSysColorBrush (COLOR_3DFACE));

                if (pContext && pContext->m_hInfoTipFont)
                {
                    hOldFont = static_cast<HFONT> (SelectObject (pdis->hDC, pContext->m_hInfoTipFont));
                }

                oldBkMode    = SetBkMode    (pdis->hDC, TRANSPARENT);
                oldTextColor = SetTextColor (pdis->hDC, GetSysColor (COLOR_WINDOWTEXT));

                DrawTextW (pdis->hDC,
                           glyph,
                           1,
                           &pdis->rcItem,
                           DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

                SetTextColor (pdis->hDC, oldTextColor);
                SetBkMode    (pdis->hDC, oldBkMode);

                if (hOldFont)
                {
                    SelectObject (pdis->hDC, hOldFont);
                }

                if (pdis->itemState & ODS_FOCUS)
                {
                    DrawFocusRect (pdis->hDC, &pdis->rcItem);
                }

                result = TRUE;
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = reinterpret_cast<LPNMHDR> (lParam);

            if (pnmhdr && (pnmhdr->code == TTN_GETDISPINFOW || pnmhdr->code == TTN_NEEDTEXTW))
            {
                // Resolve the tool's hwnd back to an IDC_*_INFO control id
                // and supply the locked infotip text via LPSTR_TEXTCALLBACK.
                NMTTDISPINFOW * pdi = reinterpret_cast<NMTTDISPINFOW *> (lParam);
                HWND            hToolHwnd = reinterpret_cast<HWND> (pdi->hdr.idFrom);
                int             toolId    = GetDlgCtrlID (hToolHwnd);

                if (IsInfoTipControlId (toolId))
                {
                    pdi->lpszText = const_cast<LPWSTR> (GetInfoTipText (toolId));
                    result        = TRUE;
                }
            }
            break;
        }

        case WM_TIMER:
            if (wParam == kInfoTipDismissTimerId)
            {
                DismissInfoTip (hDlg);
                result = TRUE;
            }
            break;

        case WM_ACTIVATE:
            // Lose-focus on the dialog dismisses any active TTF_TRACK tip.
            if (LOWORD (wParam) == WA_INACTIVE)
            {
                DismissInfoTip (hDlg);
            }
            break;
        
        case WM_DESTROY:
            OnDestroy (hDlg);
            result = TRUE;
            break;
    }

    return result;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ShowConfigDialog
//
//  Display configuration dialog for screensaver settings.
//  Modal mode (Control Panel): HWND provided via /c:<HWND>
//
////////////////////////////////////////////////////////////////////////////////

int ShowConfigDialog (HINSTANCE hInstance, const ScreenSaverModeContext & context)
{
    HRESULT         hr         = S_OK;
    DialogContext   dlgContext;
    HWND            parentHwnd = context.m_previewParentHwnd;
    INT_PTR         result     = -1;
    


    dlgContext.m_controller = std::make_unique<ConfigDialogController> (dlgContext.m_settingsProvider);
    dlgContext.m_isScreenSaverCPL = true;
    hr = dlgContext.m_controller->Initialize();
    CHRA (hr);

    result = DialogBoxParamW (hInstance,
                               MAKEINTRESOURCEW (IDD_MATRIXRAIN_SAVER_CONFIG),
                               parentHwnd,
                               ConfigDialogProc,
                               reinterpret_cast<LPARAM> (&dlgContext));

    if (result == -1)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
    }

Error:
    if (FAILED (hr))
    {
        MessageBoxW (nullptr, L"Failed to initialize configuration dialog.", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    return static_cast<int> (result);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CreateConfigDialog
//
//  Create modeless configuration dialog over running application.
//  Live overlay mode: /c without HWND - dialog shown atop animation
//  Returns dialog HWND via phDlg out parameter.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CreateConfigDialog (HINSTANCE          hInstance,
                             HWND               parentHwnd,
                             Application      * pApplication,
                             ApplicationState * pAppState,
                             HWND             * phDlg)
{
    HRESULT hr      = S_OK;
    HWND    hDlg    = nullptr;
    auto    context = std::make_unique<DialogContext>();



    context->m_controller = std::make_unique<ConfigDialogController> (pApplication->GetSettingsProvider());
    hr = context->m_controller->Initialize();
    CHRA (hr);

    hr = context->m_controller->InitializeLiveMode (pAppState);
    CHRA (hr);

    context->m_pApp              = pApplication;
    context->m_ownsContextMemory = true;

    hDlg = CreateDialogParamW (hInstance,
                               MAKEINTRESOURCEW (IDD_MATRIXRAIN_SAVER_CONFIG),
                               parentHwnd,
                               ConfigDialogProc,
                               reinterpret_cast<LPARAM> (context.get()));
    
    CWRA (hDlg);

    context.release();

    // Show the modeless dialog
    ShowWindow (hDlg, SW_SHOW);

Error:
    if (FAILED (hr))
    {
        MessageBoxW (nullptr, L"Failed to create live overlay configuration dialog.", L"Error", MB_OK | MB_ICONERROR);
        hDlg = nullptr;
    }

    *phDlg = hDlg;

    return hr;
}
