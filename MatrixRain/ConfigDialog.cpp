#include "pch.h"

#include <commctrl.h>
#include <prsht.h>
#pragma comment(lib, "comctl32.lib")

#include "ConfigDialog.h"
#include "..\MatrixRainCore\AdapterSelection.h"
#include "..\MatrixRainCore\Application.h"
#include "..\MatrixRainCore\ApplicationState.h"
#include "..\MatrixRainCore\ConfigDialogController.h"
#include "..\MatrixRainCore\ColorScheme.h"
#include "..\MatrixRainCore\CommandLine.h"
#include "..\MatrixRainCore\MonitorRenderContext.h"
#include "..\MatrixRainCore\RenderSystem.h"
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

    // T058/T059 - per-page tooltip control and shared TTF_TRACK tool whose
    // text is updated on each info-button BN_CLICKED.  Stored on the page
    // HWND via SetPropW(kPageTooltipProp) so OnInfoButtonClick / DismissInfoTip
    // can look it up without knowing the page index.

    // Larger font used to render the info-tip ⓘ glyph at 1.5x the dialog's
    // default font size in the BS_OWNERDRAW button paint path.  Created on
    // the first page WM_INITDIALOG; destroyed on sheet WM_DESTROY.
    HFONT                                     m_hInfoTipFont        = nullptr;

    // v1.5 (T029, T032, T033, contracts/propertysheet.md): frame state.
    // m_hSheet is the property-sheet frame HWND (returned by PropertySheetW
    // in modeless mode); both page procs and the title-timer reach it via
    // GetParent or via the SetProp/GetProp keyed to kSheetContextProp.
    // m_applied / m_rolledBack make the per-page PSN_APPLY / PSN_RESET
    // notifications idempotent — only the first page to see each one
    // performs the commit/rollback work.
    HWND                                      m_hSheet              = nullptr;
    bool                                      m_applied             = false;
    bool                                      m_rolledBack              = false;
    bool                                      m_isModeless              = false;
    WCHAR                                     m_perfTitleFormat[64]     = {};
    WCHAR                                     m_lastPerfTabTitle[64]    = {};
};


// v1.5 (T029): property name used to ferry DialogContext * from the
// property-sheet PSCB_INITIALIZED callback into the per-tick TimerProc
// and the page procs.  Stored on the frame HWND.
static constexpr const wchar_t kSheetContextProp[] = L"MatrixRainSheetCtx";

// v1.5 (T029): per-page tooltip stored on each page HWND so OnInfoButton*
// helpers can reach it without knowing which page they're running on.
static constexpr const wchar_t kPageTooltipProp[]  = L"MatrixRainPageToolt";

// v1.5 (T032): 1 Hz title-update timer ID (frame-scoped).
static constexpr UINT_PTR      kPerfTitleTimerId   = IDT_PERF_TITLE_TIMER;


// Sentinel uId for the single TTF_TRACK tool used by keyboard activation.
static constexpr UINT_PTR kTrackTipUId = 0xC0C00C0Cu;

// Subclass id for the info-button mouse-event relay (TTF_SUBCLASS proved
// unreliable on BS_OWNERDRAW buttons; we explicitly forward mouse events
// to the tooltip via TTM_RELAYEVENT instead).
static constexpr UINT_PTR kInfoButtonSubclassId = 0xC0C00C1Bu;

// Mini-phase 2.5 (cross-page Reset button): the property-sheet frame
// broadcasts this to every page after invoking
// `ConfigDialogController::ResetToDefaults()` so each page re-reads its
// own controls from the freshly-reset `controller->GetSettings()`.  The
// controller has already pushed the defaults through to `ApplicationState`
// for the live-preview path, so this message is strictly about UI re-sync,
// not about settings propagation.
#define WM_APP_RESET_RESYNC      (WM_APP + 50)
#define WM_APP_REPOSITION_RESET  (WM_APP + 51)




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

        case IDC_SCANLINES_INTENSITY_INFO:
            return L"How dark the scanline gaps are between bright lines. 0% disables "
                   L"the effect; 100% makes the gaps fully black.\r\n"
                   L"\r\n"
                   L"Small GPU performance impact.";

        case IDC_SCANLINES_STYLE_INFO:
            return L"Spacing of the scanlines. Low values produce many fine lines "
                   L"(modern displays); high values produce a coarse retro-CRT look.\r\n"
                   L"\r\n"
                   L"No additional GPU impact.";

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
        case IDC_SCANLINES_INTENSITY_INFO:
        case IDC_SCANLINES_STYLE_INFO:
            return true;
        default:
            return false;
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  IsDisabledGlowTooltipId (T043, FR-018, FR-019)
//
//  Catalogues every control that goes grey when Glow Enabled is OFF.
//  Used by both `RegisterDisabledGlowTooltipRects` (which registers a
//  rect-based parent-relative TOOLINFO per control) AND the
//  TTN_NEEDTEXTW/TTN_GETDISPINFOW branch in `PageDlgProc` (which
//  recognises the uId as a sentinel and supplies the per-tab text).
//  Disabled controls don't fire WM_MOUSEMOVE, so mouse events bubble to
//  the parent dialog — the rect-based tools on the parent catch the
//  hover and show the explanatory tip.  When glow is ON, the controls
//  consume their own mouse events and the parent rect tools never fire.
//
////////////////////////////////////////////////////////////////////////////////

static bool IsDisabledGlowTooltipId (int id)
{
    switch (id)
    {
        // Visuals tab — glow intensity / glow size trios.
        case IDC_GLOWINTENSITY_SLIDER:
        case IDC_GLOWINTENSITY_LABEL:
        case IDC_GLOWINTENSITY_INFO:
        case IDC_GLOWSIZE_SLIDER:
        case IDC_GLOWSIZE_LABEL:
        case IDC_GLOWSIZE_INFO:

        // Performance tab — quality preset + glow-passes / -resolution /
        // -smoothness trios (matches ApplyGlowEnabledUI's coverage).
        case IDC_QUALITY_PRESET_SLIDER:
        case IDC_QUALITY_PRESET_LABEL:
        case IDC_QUALITY_PRESET_INFO:
        case IDC_GLOWPASSES_PROMPT:
        case IDC_GLOWPASSES_SLIDER:
        case IDC_GLOWPASSES_LABEL:
        case IDC_GLOWPASSES_INFO:
        case IDC_GLOWRES_PROMPT:
        case IDC_GLOWRES_SLIDER:
        case IDC_GLOWRES_LABEL:
        case IDC_GLOWRES_INFO:
        case IDC_GLOWSMOOTH_PROMPT:
        case IDC_GLOWSMOOTH_SLIDER:
        case IDC_GLOWSMOOTH_LABEL:
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
        IDC_SCANLINES_INTENSITY_INFO,
        IDC_SCANLINES_STYLE_INFO,
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


    // T043 (US2, FR-018, FR-019): rect-based tooltip tools on the parent
    // dialog for every control that goes grey when Glow Enabled is OFF.
    // Disabled children don't fire WM_MOUSEMOVE, so the events bubble to
    // the parent and the parent-relative rect tool catches them — when
    // glow is ON the controls consume their own mouse events and these
    // rect tools never activate (silently dormant).  TTF_SUBCLASS makes
    // the tooltip subclass the parent so we don't need to manually relay
    // WM_MOUSEMOVE.  uId is the control's IDC_* so the TTN_NEEDTEXTW
    // branch can identify which tool fired without an HWND lookup.
    for (int id = 1000; id < 1100; id++)
    {
        if (!IsDisabledGlowTooltipId (id))
        {
            continue;
        }

        HWND hCtrl = GetDlgItem (hDlg, id);

        if (!hCtrl)
        {
            // Control isn't on this page (e.g., the Performance-tab
            // sliders won't exist when we're initialising the Visuals
            // page).  Skip — the other page's call registers its own.
            continue;
        }


        RECT  childRect = {};

        GetWindowRect (hCtrl, &childRect);

        POINT topLeft     = { childRect.left,  childRect.top };
        POINT bottomRight = { childRect.right, childRect.bottom };

        ScreenToClient (hDlg, &topLeft);
        ScreenToClient (hDlg, &bottomRight);

        TOOLINFOW ti  = { sizeof (TOOLINFOW) };
        ti.uFlags     = TTF_SUBCLASS;
        ti.hwnd       = hDlg;
        ti.uId        = static_cast<UINT_PTR> (id);
        ti.rect.left   = topLeft.x;
        ti.rect.top    = topLeft.y;
        ti.rect.right  = bottomRight.x;
        ti.rect.bottom = bottomRight.y;
        ti.lpszText   = LPSTR_TEXTCALLBACKW;

        SendMessageW (hTooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM> (&ti));
    }

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
    HWND hTooltip = static_cast<HWND> (GetPropW (hDlg, kPageTooltipProp));

    if (!hTooltip)
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

    SendMessageW (hTooltip, TTM_UPDATETIPTEXTW, 0, (LPARAM) &ti);

    SendMessageW (hTooltip,
                  TTM_TRACKPOSITION,
                  0,
                  MAKELPARAM (btnRect.right + 4, btnRect.bottom + 2));

    SendMessageW (hTooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM) &ti);

    SetTimer (hDlg, kInfoTipDismissTimerId, 5000, nullptr);
}




static void DismissInfoTip (HWND hDlg)
{
    HWND hTooltip = static_cast<HWND> (GetPropW (hDlg, kPageTooltipProp));

    if (!hTooltip)
    {
        return;
    }

    TOOLINFOW ti = { sizeof (TOOLINFOW) };
    ti.hwnd      = hDlg;
    ti.uId       = kTrackTipUId;

    SendMessageW (hTooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM) &ti);
    KillTimer (hDlg, kInfoTipDismissTimerId);
}




static std::wstring FormatPercentLabel (int sliderId, int value)
{
    // T041 (US2, FR-007): removed the legacy "0% (glow disabled)" branch.
    // Glow on/off now lives on IDC_GLOW_ENABLED_CHECK; the slider min is
    // back to 1 (see ScreenSaverSettings::MIN_GLOW_INTENSITY_PERCENT).
    (void) sliderId;
    return std::format (L"{}%", value);
}





////////////////////////////////////////////////////////////////////////////////
//
//  ApplyGlowEnabledUI (T042, FR-016, FR-017, research.md R7)
//
//  Mirror m_glowEnabled into EnableWindow on every glow-dependent control
//  across both property-sheet pages.  Cross-tab reach is safe because
//  PSP_PREMATURE forces both page HWNDs to exist immediately after
//  PropertySheetW returns — see plan.md "Property-sheet flags".
//
//  Per spec the Visuals-tab Glow Intensity / Glow Size trios disable, plus
//  the Performance-tab Quality Preset / Glow Passes / Glow Resolution /
//  Glow Smoothness trios.  Static "Glow intensity:" / "Glow size:" prompt
//  labels are IDC_STATIC (no individual ID) so they can't be greyed
//  programmatically; the value labels and info buttons carry the visual
//  cue instead.
//
//  T043 (per-tab tooltip on disabled controls) is deferred — Windows
//  tooltips don't fire on WS_DISABLED controls without a transparent
//  parent-relay tooltip per page, and the greyed-out controls already
//  convey "disabled" clearly.  Tracked as follow-up.
//
////////////////////////////////////////////////////////////////////////////////

static void ApplyGlowEnabledUI (HWND hSheet, bool enabled)
{
    if (!hSheet)
    {
        return;
    }


    HWND hVisuals = PropSheet_IndexToHwnd (hSheet, 0);
    HWND hPerf    = PropSheet_IndexToHwnd (hSheet, 1);

    auto enableIfPresent = [enabled] (HWND hPage, int id)
    {
        if (hPage)
        {
            HWND hCtrl = GetDlgItem (hPage, id);

            if (hCtrl)
            {
                EnableWindow (hCtrl, enabled);
            }
        }
    };


    // Visuals tab — Glow Intensity + Glow Size trios.
    enableIfPresent (hVisuals, IDC_GLOWINTENSITY_SLIDER);
    enableIfPresent (hVisuals, IDC_GLOWINTENSITY_LABEL);
    enableIfPresent (hVisuals, IDC_GLOWINTENSITY_INFO);
    enableIfPresent (hVisuals, IDC_GLOWSIZE_SLIDER);
    enableIfPresent (hVisuals, IDC_GLOWSIZE_LABEL);
    enableIfPresent (hVisuals, IDC_GLOWSIZE_INFO);

    // Performance tab — Quality Preset + Glow Passes/Resolution/Smoothness trios.
    enableIfPresent (hPerf,    IDC_QUALITY_PRESET_SLIDER);
    enableIfPresent (hPerf,    IDC_QUALITY_PRESET_LABEL);
    enableIfPresent (hPerf,    IDC_QUALITY_PRESET_INFO);
    enableIfPresent (hPerf,    IDC_GLOWPASSES_SLIDER);
    enableIfPresent (hPerf,    IDC_GLOWPASSES_LABEL);
    enableIfPresent (hPerf,    IDC_GLOWPASSES_INFO);
    enableIfPresent (hPerf,    IDC_GLOWPASSES_PROMPT);
    enableIfPresent (hPerf,    IDC_GLOWRES_SLIDER);
    enableIfPresent (hPerf,    IDC_GLOWRES_LABEL);
    enableIfPresent (hPerf,    IDC_GLOWRES_INFO);
    enableIfPresent (hPerf,    IDC_GLOWRES_PROMPT);
    enableIfPresent (hPerf,    IDC_GLOWSMOOTH_SLIDER);
    enableIfPresent (hPerf,    IDC_GLOWSMOOTH_LABEL);
    enableIfPresent (hPerf,    IDC_GLOWSMOOTH_INFO);
    enableIfPresent (hPerf,    IDC_GLOWSMOOTH_PROMPT);
}





////////////////////////////////////////////////////////////////////////////////
//
//  ApplyScanlinesEnabledUI (T054, FR-028b) — mirror the Scanlines Enabled
//  checkbox state into EnableWindow on the two scanline sliders + their
//  labels + info buttons on the Visuals page.  Same shape as the glow
//  helper above, scoped to one tab.
//
////////////////////////////////////////////////////////////////////////////////

static void ApplyScanlinesEnabledUI (HWND hPage, bool enabled)
{
    if (!hPage)
    {
        return;
    }


    auto enableIfPresent = [hPage, enabled] (int id)
    {
        HWND hCtrl = GetDlgItem (hPage, id);

        if (hCtrl)
        {
            EnableWindow (hCtrl, enabled);
        }
    };


    enableIfPresent (IDC_SCANLINES_INTENSITY_SLIDER);
    enableIfPresent (IDC_SCANLINES_INTENSITY_VALUE);
    enableIfPresent (IDC_SCANLINES_INTENSITY_INFO);
    enableIfPresent (IDC_SCANLINES_INTENSITY_PROMPT);
    enableIfPresent (IDC_SCANLINES_STYLE_SLIDER);
    enableIfPresent (IDC_SCANLINES_STYLE_VALUE);
    enableIfPresent (IDC_SCANLINES_STYLE_INFO);
    enableIfPresent (IDC_SCANLINES_STYLE_PROMPT);
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
    { L"green",  L"Green"     },
    { L"blue",   L"Blue"      },
    { L"red",    L"Red"       },
    { L"amber",  L"Amber"     },
    { L"cycle",  L"Cycle"     },
    { L"custom", L"Custom\u2026" }, // 6th entry per T063 (FR-006 / FR-029)
};

// US5 (T064): the index of "Custom" in s_colorSchemeEntries — used by
// OnColorSchemeChange to recognise the user picked it (opens chooser)
// and by the combo subclass for same-item re-click detection.
static constexpr int kCustomColorComboIndex = 5;

// US5 (T064, research.md R4): per-page tracking of the last combo
// selection observed by the subclass.  Lets us distinguish "user
// picked a different item" (CBN_SELCHANGE will fire) from "user re-
// clicked the same Custom entry" (CBN_SELCHANGE WON'T fire, but we
// still want to re-open the chooser).  Stored on the page HWND via
// SetPropW so the subclass + WM_COMMAND handler can share state.
static constexpr const wchar_t kLastColorComboIndexProp[] = L"MatrixRainLastColIdx";

// US5 (T064): WM_APP messages used by the combo subclass to defer
// chooser invocation out of the comctl mouse/key dispatch (calling
// ChooseColorW inside the message handler would deadlock the modal
// owner).  Posted from the subclass, handled in PageDlgProc.
#define WM_APP_OPEN_CUSTOM_COLOR_CHOOSER (WM_APP + 51)




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
    int initialIndex = ColorSchemeKeyToIndex (currentScheme);


    for (const ColorSchemeEntry & entry : s_colorSchemeEntries)
    {
        SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_ADDSTRING, 0, (LPARAM) entry.label);
    }

    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_SETCURSEL, initialIndex, 0);

    // T064 (US5, research.md R4): track the last selected index so the
    // combo subclass can detect same-item re-click on Custom (CBN_SELCHANGE
    // doesn't fire when the user re-commits the already-selected entry,
    // but we still want to re-open the chooser per the colour-picker UX
    // convention).  Stored +1 so a missing/cleared prop reads as 0 ≠ 0+1.
    SetPropW (hDlg, kLastColorComboIndexProp,
              reinterpret_cast<HANDLE> (static_cast<INT_PTR> (initialIndex + 1)));
}




////////////////////////////////////////////////////////////////////////////////
//
//  OpenCustomColorChooser (T065, FR-029, FR-030, FR-031, FR-032, FR-035)
//
//  Modal Win32 ChooseColorW.  Pre-populates rgbResult from the controller's
//  current m_customColor (RGB(0,255,0) on first launch), seeds lpCustColors
//  with the 16-swatch palette from settings, opens with CC_FULLOPEN so the
//  user sees the full editor pane.  On OK: unconditionally writes back the
//  palette (FR-035 -- swatch edits persist even on outer Cancel), updates
//  the active CustomColor, and switches scheme to Custom for live preview.
//  On Cancel: no writes.  Returns true iff the user clicked OK.
//
////////////////////////////////////////////////////////////////////////////////

static bool OpenCustomColorChooser (HWND hOwnerPage)
{
    ConfigDialogController * pController = GetControllerFromDialog (hOwnerPage);



    if (!pController)
    {
        return false;
    }


    ScreenSaverSettings  workingPalette = pController->GetSettings();
    CHOOSECOLORW         cc             = {};
    bool                 accepted       = false;

    cc.lStructSize  = sizeof (cc);
    cc.hwndOwner    = GetParent (hOwnerPage) ? GetParent (hOwnerPage) : hOwnerPage;
    cc.rgbResult    = workingPalette.m_customColor;
    cc.lpCustColors = workingPalette.m_customColorPalette.data();
    cc.Flags        = CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR;

    if (ChooseColorW (&cc))
    {
        // Palette FIRST so a subsequent commit (Apply) persists swatch
        // edits even if the user later switches the scheme away from
        // Custom (FR-035 unconditional persistence carve-out).
        pController->SetCustomColorPalette (workingPalette.m_customColorPalette);

        pController->UpdateCustomColor (cc.rgbResult);
        pController->UpdateColorScheme (L"custom");

        accepted = true;
    }

    return accepted;
}




////////////////////////////////////////////////////////////////////////////////
//
//  (Removed ColorComboSubclass — see commit history.  The previous attempt
//  to catch "same Custom item re-click" via WM_LBUTTONUP fired spuriously
//  every time the dropdown was opened, making it impossible to pick a
//  different preset once Custom was selected.  CBN_SELCHANGE alone is now
//  the trigger: re-opening the chooser requires a different combo
//  selection followed by re-selecting Custom, which is acceptable for the
//  rare edit-existing-custom case.)
//
////////////////////////////////////////////////////////////////////////////////



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

// Forward declarations for the colour swatch helpers (defined after this
// function in the file; OnInitDialog needs them to start the cycle timer
// when the initial scheme is Cycle).
static void UpdateCycleTimerForCurrentScheme (HWND hDlg);
static void InvalidateColorSwatch            (HWND hDlg);


static BOOL OnInitDialog (HWND hDlg, LPARAM initParam)
{
    HRESULT                     hr           = S_OK;
    BOOL                        fSuccess     = FALSE;
    DialogContext             * pContext     = nullptr;
    const ScreenSaverSettings * pSettings    = nullptr;
    PROPSHEETPAGEW            * pPsp         = reinterpret_cast<PROPSHEETPAGEW *> (initParam);


    // v1.5 (T029): the page proc receives the PROPSHEETPAGE pointer in
    // WM_INITDIALOG's lParam.  We tucked the DialogContext * into psp->lParam
    // at sheet build time (see BuildPropSheetHeader).
    CBRAEx (pPsp != nullptr, E_UNEXPECTED);

    pContext = reinterpret_cast<DialogContext *> (pPsp->lParam);
    CBRAEx (pContext != nullptr && pContext->m_controller != nullptr, E_UNEXPECTED);

    SetWindowLongPtr (hDlg, DWLP_USER, reinterpret_cast<LONG_PTR> (pContext));

    // v1.5 (T029): centering is owned by the property-sheet frame's
    // PSCB_INITIALIZED callback (the frame is what the user sees moving),
    // not by individual pages.

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

    // Color combo: subclass removed; CBN_SELCHANGE drives the chooser.
    // The kLastColorComboIndexProp prop is still useful (read by the
    // chooser-cancel revert path) and is maintained by InitializeColor-
    // SchemeCombo / ResyncPageFromSettings / OnColorSchemeChange.

    // Quality preset slider (0=Low, 1=Medium, 2=High, 3=Custom).
    SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETRANGE,   TRUE, MAKELPARAM (0, 3));
    SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS,     TRUE, static_cast<int> (pSettings->m_qualityPreset));
    SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (pSettings->m_qualityPreset));

    InitializePassesSlider      (hDlg, pSettings->m_advancedValues.m_blurPasses);
    InitializeResolutionSlider  (hDlg, static_cast<int> (pSettings->m_advancedValues.m_bloomResolutionDivisor));
    InitializeSmoothnessSlider  (hDlg, static_cast<int> (pSettings->m_advancedValues.m_blurTaps));

    // v1.5 (T054, US3): scanline controls.  Intensity/Style are 1..100
    // sliders matching the v1.4 percentage-slider pattern.  Tick freq 5
    // on both so the 100-step range lands 21 visible ticks.
    SendDlgItemMessageW (hDlg, IDC_SCANLINES_INTENSITY_SLIDER, TBM_SETRANGE,    TRUE, MAKELPARAM (ScreenSaverSettings::MIN_SCANLINES_INTENSITY_PERCENT, ScreenSaverSettings::MAX_SCANLINES_INTENSITY_PERCENT));
    SendDlgItemMessageW (hDlg, IDC_SCANLINES_INTENSITY_SLIDER, TBM_SETTICFREQ,  5, 0);
    SendDlgItemMessageW (hDlg, IDC_SCANLINES_INTENSITY_SLIDER, TBM_SETPOS,      TRUE, pSettings->m_scanlinesIntensity);
    SetDlgItemTextW     (hDlg, IDC_SCANLINES_INTENSITY_VALUE,  std::format (L"{}%", pSettings->m_scanlinesIntensity).c_str());

    SendDlgItemMessageW (hDlg, IDC_SCANLINES_STYLE_SLIDER,     TBM_SETRANGE,    TRUE, MAKELPARAM (ScreenSaverSettings::MIN_SCANLINES_STYLE, ScreenSaverSettings::MAX_SCANLINES_STYLE));
    SendDlgItemMessageW (hDlg, IDC_SCANLINES_STYLE_SLIDER,     TBM_SETTICFREQ,  5, 0);
    SendDlgItemMessageW (hDlg, IDC_SCANLINES_STYLE_SLIDER,     TBM_SETPOS,      TRUE, pSettings->m_scanlinesStyle);
    SetDlgItemTextW     (hDlg, IDC_SCANLINES_STYLE_VALUE,      std::format (L"{}", pSettings->m_scanlinesStyle).c_str());

    CheckDlgButton      (hDlg, IDC_SCANLINES_ENABLED_CHECK,
                         pSettings->m_scanlinesEnabled ? BST_CHECKED : BST_UNCHECKED);

    // Mirror initial scanlines-enabled state into the slider/info enable flags.
    ApplyScanlinesEnabledUI (hDlg, pSettings->m_scanlinesEnabled);

    // Start the colour swatch cycle timer if the initial scheme is Cycle
    // (no-op on Performance page, which has no swatch control).
    UpdateCycleTimerForCurrentScheme (hDlg);

    // Per-page tooltip surface for the IDC_*_INFO indicators (only the
    // info buttons actually present on this page get registered tools).
    {
        HWND hTooltip = CreateAndRegisterTooltip (hDlg);

        if (hTooltip)
        {
            SetPropW (hDlg, kPageTooltipProp, hTooltip);
        }
    }

    // Create the 1.5x-size font used by the owner-drawn ⓘ glyphs ONCE per
    // sheet — both pages share the same font.  Created on whichever page
    // initialises first (typically Visuals due to PSP_PREMATURE order).
    if (!pContext->m_hInfoTipFont)
    {
        HFONT hDialogFont = reinterpret_cast<HFONT> (SendMessageW (hDlg, WM_GETFONT, 0, 0));
        LOGFONTW lf       = {};

        if (hDialogFont && GetObjectW (hDialogFont, sizeof (lf), &lf))
        {
            lf.lfHeight = (lf.lfHeight < 0)
                            ? -static_cast<LONG> ((-lf.lfHeight) * 3 / 2)
                            :  static_cast<LONG> ( lf.lfHeight * 3 / 2);

            pContext->m_hInfoTipFont = CreateFontIndirectW (&lf);
        }
    }
    
    CheckDlgButton (hDlg, IDC_STARTFULLSCREEN_CHECK, pSettings->m_startFullscreen     ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_MULTIMONITOR_CHECK,    pSettings->m_multiMonitorEnabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWDEBUG_CHECK,       pSettings->m_showDebugStats      ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_GLOW_ENABLED_CHECK,    pSettings->m_glowEnabled         ? BST_CHECKED : BST_UNCHECKED);

    // Hide fullscreen checkbox in screensaver CPL mode — screensaver always forces fullscreen
    if (pContext->m_isScreenSaverCPL)
    {
        ShowWindow (GetDlgItem (hDlg, IDC_STARTFULLSCREEN_CHECK), SW_HIDE);
    }

    // T044 (US2, research.md R7): mirror initial glow-enabled state into
    // EnableWindow on both pages' glow-dependent controls.  Safe to call
    // from either page proc because PSP_PREMATURE guarantees both HWNDs
    // exist; the function no-ops if PropSheet_IndexToHwnd returns null on
    // the first page's init (the second page's init re-applies).
    {
        HWND hSheet = GetParent (hDlg);

        if (hSheet)
        {
            ApplyGlowEnabledUI (hSheet, pSettings->m_glowEnabled);
        }
    }
    
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

        case IDC_SCANLINES_INTENSITY_SLIDER:
            pController->UpdateScanlinesIntensity (pos);
            SetDlgItemTextW (hDlg, IDC_SCANLINES_INTENSITY_VALUE, std::format (L"{}%", pos).c_str());
            break;

        case IDC_SCANLINES_STYLE_SLIDER:
            pController->UpdateScanlinesStyle (pos);
            SetDlgItemTextW (hDlg, IDC_SCANLINES_STYLE_VALUE, std::format (L"{}", pos).c_str());
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
//  ColorSwatch helpers (v1.5)
//
//  Paint the owner-draw swatch (IDC_COLOR_SWATCH) on the Visuals page so
//  it reflects the currently-selected colour scheme.  Cycle mode is
//  driven by a 30Hz timer (IDT_COLOR_CYCLE_TIMER) that just invalidates
//  the swatch; the paint code re-queries GetColorRGB with a fresh time.
//
////////////////////////////////////////////////////////////////////////////////

static constexpr UINT_PTR kColorCycleTimerId      = IDT_COLOR_CYCLE_TIMER;
static constexpr UINT     kColorCycleTickInterval = 33;  // ~30 Hz; matches rain refresh well enough for a 28x12 swatch




static COLORREF ResolveSwatchColor (HWND hDlg)
{
    DialogContext * pContext = GetDialogContext (hDlg);


    if (!pContext || !pContext->m_controller)
    {
        return RGB (0, 255, 0);
    }

    const ScreenSaverSettings & settings = pContext->m_controller->GetSettings();
    int                         idx      = ColorSchemeKeyToIndex (settings.m_colorSchemeKey);


    if (idx == kCustomColorComboIndex)
    {
        return settings.m_customColor;
    }

    ColorScheme scheme       = (idx == 4) ? ColorScheme::ColorCycle
                                          : static_cast<ColorScheme> (idx);
    float       elapsedTime  = static_cast<float> (GetTickCount64()) / 1000.0f;
    Color4      c            = GetColorRGB (scheme, elapsedTime);


    return RGB (static_cast<BYTE> (std::clamp (c.r, 0.0f, 1.0f) * 255.0f + 0.5f),
                static_cast<BYTE> (std::clamp (c.g, 0.0f, 1.0f) * 255.0f + 0.5f),
                static_cast<BYTE> (std::clamp (c.b, 0.0f, 1.0f) * 255.0f + 0.5f));
}




static void DrawColorSwatch (HWND hDlg, LPDRAWITEMSTRUCT pdis)
{
    COLORREF rgb    = ResolveSwatchColor (hDlg);
    HBRUSH   hBrush = CreateSolidBrush (rgb);


    if (hBrush)
    {
        FillRect (pdis->hDC, &pdis->rcItem, hBrush);
        DeleteObject (hBrush);
    }
}




static void UpdateCycleTimerForCurrentScheme (HWND hDlg)
{
    DialogContext * pContext = GetDialogContext (hDlg);


    if (!pContext || !pContext->m_controller || !GetDlgItem (hDlg, IDC_COLOR_SWATCH))
    {
        return;
    }

    int idx = ColorSchemeKeyToIndex (pContext->m_controller->GetSettings().m_colorSchemeKey);


    // Cycle entry is index 4 (kCustomColorComboIndex == 5).
    if (idx == 4)
    {
        SetTimer (hDlg, kColorCycleTimerId, kColorCycleTickInterval, nullptr);
    }
    else
    {
        KillTimer (hDlg, kColorCycleTimerId);
    }
}




static void InvalidateColorSwatch (HWND hDlg)
{
    HWND hSwatch = GetDlgItem (hDlg, IDC_COLOR_SWATCH);


    if (hSwatch)
    {
        InvalidateRect (hSwatch, nullptr, FALSE);
    }
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
    int                      priorIndex  = 0;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    index = (int) SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_GETCURSEL, 0, 0);

    CBRAEx (index >= 0 && index < static_cast<int> (ARRAYSIZE (s_colorSchemeEntries)), E_UNEXPECTED);

    // T064 (US5, FR-029): selecting "Custom..." opens the chooser before
    // committing the scheme.  If the user cancels, revert the combo to
    // the previously selected scheme — they explicitly opted out.  The
    // chooser invocation runs out-of-line via WM_APP_OPEN_CUSTOM_COLOR_CHOOSER
    // so the modal doesn't re-enter the CBN_SELCHANGE dispatch.
    priorIndex = static_cast<int> (reinterpret_cast<INT_PTR> (GetPropW (hDlg, kLastColorComboIndexProp))) - 1;

    if (index == kCustomColorComboIndex)
    {
        PostMessageW (hDlg, WM_APP_OPEN_CUSTOM_COLOR_CHOOSER, 0, 0);
        // Don't update controller yet — defer to the chooser-OK path.
        // Last-index is NOT updated here either; the post-handler updates
        // it after the chooser closes (revert path or accept path).
    }
    else
    {
        pController->UpdateColorScheme (s_colorSchemeEntries[index].key);

        SetPropW (hDlg, kLastColorComboIndexProp,
                  reinterpret_cast<HANDLE> (static_cast<INT_PTR> (index + 1)));
    }

    UpdateCycleTimerForCurrentScheme (hDlg);
    InvalidateColorSwatch            (hDlg);

    UNREFERENCED_PARAMETER (priorIndex);

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






////////////////////////////////////////////////////////////////////////////////
//
//  OnResetButton
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  ResyncPageFromSettings — re-read every control's value from the supplied
//  settings and update the UI.  Mini-phase 2.5: invoked on the
//  `WM_APP_RESET_RESYNC` broadcast from the frame's Reset button so a
//  single click can refresh controls on both tabs without either page
//  having to know what's on the other.  Page-agnostic by design: each
//  `GetDlgItem` call returns NULL for IDs not present on this page, so
//  `SendDlgItemMessageW` / `SetDlgItemTextW` / `CheckDlgButton` are
//  silent no-ops for the other tab's controls.
//
////////////////////////////////////////////////////////////////////////////////

static void ResyncPageFromSettings (HWND hDlg, const ScreenSaverSettings & settings)
{
    int           schemeIndex = 0;
    DialogContext * pCtx      = GetDialogContext (hDlg);



    // Visuals tab — percent sliders and color combo (silently skipped on
    // the Performance page since the controls aren't present there).
    SendDlgItemMessageW (hDlg, IDC_DENSITY_SLIDER,       TBM_SETPOS, TRUE, settings.m_densityPercent);
    SetDlgItemTextW     (hDlg, IDC_DENSITY_LABEL,        std::format (L"{}%", settings.m_densityPercent).c_str());

    SendDlgItemMessageW (hDlg, IDC_ANIMSPEED_SLIDER,     TBM_SETPOS, TRUE, settings.m_animationSpeedPercent);
    SetDlgItemTextW     (hDlg, IDC_ANIMSPEED_LABEL,      std::format (L"{}%", settings.m_animationSpeedPercent).c_str());

    SendDlgItemMessageW (hDlg, IDC_GLOWINTENSITY_SLIDER, TBM_SETPOS, TRUE, settings.m_glowIntensityPercent);
    SetDlgItemTextW     (hDlg, IDC_GLOWINTENSITY_LABEL,  std::format (L"{}%", settings.m_glowIntensityPercent).c_str());

    SendDlgItemMessageW (hDlg, IDC_GLOWSIZE_SLIDER,      TBM_SETPOS, TRUE, settings.m_glowSizePercent);
    SetDlgItemTextW     (hDlg, IDC_GLOWSIZE_LABEL,       std::format (L"{}%", settings.m_glowSizePercent).c_str());

    schemeIndex = ColorSchemeKeyToIndex (settings.m_colorSchemeKey);
    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_SETCURSEL, schemeIndex, 0);

    // Refresh the last-selection tracker so the next OnColorSchemeChange
    // reads a sane prior value (the Reset button doesn't pass through
    // OnColorSchemeChange).
    if (GetDlgItem (hDlg, IDC_COLORSCHEME_COMBO))
    {
        SetPropW (hDlg, kLastColorComboIndexProp,
                  reinterpret_cast<HANDLE> (static_cast<INT_PTR> (schemeIndex + 1)));
    }

    // Scanline controls (Visuals tab).
    SendDlgItemMessageW (hDlg, IDC_SCANLINES_INTENSITY_SLIDER, TBM_SETPOS, TRUE, settings.m_scanlinesIntensity);
    SetDlgItemTextW     (hDlg, IDC_SCANLINES_INTENSITY_VALUE,  std::format (L"{}%", settings.m_scanlinesIntensity).c_str());
    SendDlgItemMessageW (hDlg, IDC_SCANLINES_STYLE_SLIDER,     TBM_SETPOS, TRUE, settings.m_scanlinesStyle);
    SetDlgItemTextW     (hDlg, IDC_SCANLINES_STYLE_VALUE,      std::format (L"{}", settings.m_scanlinesStyle).c_str());
    CheckDlgButton      (hDlg, IDC_SCANLINES_ENABLED_CHECK,
                         settings.m_scanlinesEnabled ? BST_CHECKED : BST_UNCHECKED);

    if (GetDlgItem (hDlg, IDC_SCANLINES_ENABLED_CHECK))
    {
        ApplyScanlinesEnabledUI (hDlg, settings.m_scanlinesEnabled);
    }

    // Performance tab — multimon, glow toggle, GPU combo, quality cluster,
    // show-metrics toggle.
    CheckDlgButton (hDlg, IDC_MULTIMONITOR_CHECK,    settings.m_multiMonitorEnabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWDEBUG_CHECK,       settings.m_showDebugStats      ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_GLOW_ENABLED_CHECK,    settings.m_glowEnabled         ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_STARTFULLSCREEN_CHECK, settings.m_startFullscreen     ? BST_CHECKED : BST_UNCHECKED);

    // GPU combo: re-resolve the default-marked entry in the populated list.
    if (pCtx && GetDlgItem (hDlg, IDC_GPU_COMBO))
    {
        int defaultIdx = 0;


        for (size_t i = 0; i < pCtx->m_gpuAdapterDescriptions.size(); i++)
        {
            if (pCtx->m_gpuAdapterDescriptions[i] == settings.m_gpuAdapter)
            {
                defaultIdx = static_cast<int> (i);
                break;
            }
        }

        SendDlgItemMessageW (hDlg, IDC_GPU_COMBO, CB_SETCURSEL, defaultIdx, 0);
    }

    SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS, TRUE, static_cast<int> (settings.m_qualityPreset));
    SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (settings.m_qualityPreset));

    InitializePassesSlider     (hDlg, settings.m_advancedValues.m_blurPasses);
    InitializeResolutionSlider (hDlg, static_cast<int> (settings.m_advancedValues.m_bloomResolutionDivisor));
    InitializeSmoothnessSlider (hDlg, static_cast<int> (settings.m_advancedValues.m_blurTaps));
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

        case IDC_GLOW_ENABLED_CHECK:
        {
            // T044 (US2, FR-016, FR-017): toggle drives the controller AND
            // immediately propagates EnableWindow across both tabs.
            bool                     enabled    = (IsDlgButtonChecked (hDlg, IDC_GLOW_ENABLED_CHECK) == BST_CHECKED);
            ConfigDialogController * pCtrl      = GetControllerFromDialog (hDlg);
            HWND                     hSheet     = GetParent (hDlg);

            if (pCtrl)
            {
                pCtrl->UpdateGlowEnabled (enabled);
            }

            ApplyGlowEnabledUI (hSheet, enabled);
            break;
        }

        case IDC_SCANLINES_ENABLED_CHECK:
        {
            // T054 (US3, FR-028b): toggle Scanlines Enabled, mirror into
            // controller, and grey/enable the two scanline sliders + their
            // info buttons on the same (Visuals) page.
            bool                     enabled = (IsDlgButtonChecked (hDlg, IDC_SCANLINES_ENABLED_CHECK) == BST_CHECKED);
            ConfigDialogController * pCtrl   = GetControllerFromDialog (hDlg);

            if (pCtrl)
            {
                pCtrl->UpdateScanlinesEnabled (enabled);
            }

            ApplyScanlinesEnabledUI (hDlg, enabled);
            break;
        }
    }

Error:
    return fSuccess;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnDestroy — per-page cleanup only.  The shared DialogContext (controller,
//  app pointer, info-tip font) is owned by the property-sheet frame and
//  freed in PropSheetCallback / DestroySheetContext, not here.
//
////////////////////////////////////////////////////////////////////////////////

static void OnDestroy (HWND hDlg)
{
    HWND hTooltip = static_cast<HWND> (GetPropW (hDlg, kPageTooltipProp));


    if (hTooltip)
    {
        DestroyWindow (hTooltip);
        RemovePropW   (hDlg, kPageTooltipProp);
    }

    SetWindowLongPtr (hDlg, DWLP_USER, 0);
}





////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogProc
//
//  Page dialog procedure for IDD_VISUALS_PAGE / IDD_PERFORMANCE_PAGE.
//  v1.5 (T029, T030): both pages share this proc — control handlers
//  dispatch by ID, so a page only handles the controls actually present
//  in its template.  PSN_APPLY / PSN_RESET notifications (frame-owned
//  OK / Cancel buttons) are translated into CommitLiveMode /
//  CancelLiveMode here, with idempotency guards on DialogContext so the
//  second page to see the notification is a no-op.
//
////////////////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK PageDlgProc (HWND   hDlg,
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
            LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT> (lParam);


            // Owner-draw paint for the colour swatch next to the scheme combo.
            if (pdis && pdis->CtlType == ODT_STATIC && pdis->CtlID == IDC_COLOR_SWATCH)
            {
                DrawColorSwatch (hDlg, pdis);
                result = TRUE;
                break;
            }

            // Owner-draw paint for the ⓘ info indicators.  No button frame:
            // we draw only the glyph (transparent background, 1.5x font).
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

            if (!pnmhdr)
            {
                break;
            }

            // Tooltip text callback — supplies the locked infotip string.
            if (pnmhdr->code == TTN_GETDISPINFOW || pnmhdr->code == TTN_NEEDTEXTW)
            {
                NMTTDISPINFOW * pdi    = reinterpret_cast<NMTTDISPINFOW *> (lParam);
                int             toolId = static_cast<int> (pdi->hdr.idFrom);

                // T043 (US2, FR-018, FR-019): rect-based parent tools for
                // disabled glow controls.  uId is the IDC_* directly (no
                // HWND indirection).  Per-tab text — the Visuals tab points
                // users at the Performance tab where the actual toggle lives.
                if (IsDisabledGlowTooltipId (toolId))
                {
                    bool isVisualsPage = GetDlgItem (hDlg, IDC_GLOWINTENSITY_SLIDER) != nullptr;

                    pdi->lpszText = const_cast<LPWSTR> (isVisualsPage
                                                        ? L"Glow is disabled on the performance tab."
                                                        : L"Glow is disabled.");
                    result = TRUE;
                    break;
                }

                HWND hToolHwnd = reinterpret_cast<HWND> (pdi->hdr.idFrom);
                int  ctrlId    = GetDlgCtrlID (hToolHwnd);

                if (IsInfoTipControlId (ctrlId))
                {
                    pdi->lpszText = const_cast<LPWSTR> (GetInfoTipText (ctrlId));
                    result        = TRUE;
                }
                break;
            }

            // v1.5 (T030, T033, FR-004a, contracts/propertysheet.md):
            // property-sheet OK / Cancel notifications.
            DialogContext          * pContext    = GetDialogContext (hDlg);
            ConfigDialogController * pController = pContext ? pContext->m_controller.get() : nullptr;

            if (!pContext || !pController)
            {
                break;
            }

            switch (pnmhdr->code)
            {
                case PSN_APPLY:
                    if (!pContext->m_applied)
                    {
                        pContext->m_applied = true;

                        if (pController->IsLiveMode())
                        {
                            pController->CommitLiveMode();
                        }
                        else
                        {
                            pController->ApplyChanges();
                        }

                        // Modeless sheets: do NOT destroy here.  The
                        // canonical pattern is for the page to acknowledge
                        // PSN_APPLY (PSNRET_NOERROR) and let comctl32 mark
                        // the sheet "closed" by making PropSheet_GetCurrent-
                        // PageHwnd return NULL.  The message loop polls
                        // that and DestroyWindow's the sheet from outside
                        // any notification dispatch.  Destroying from in
                        // here re-enters the subclass through cascading
                        // WM_DESTROY traffic.
                    }
                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    result = TRUE;
                    break;

                case PSN_RESET:
                    if (!pContext->m_rolledBack)
                    {
                        pContext->m_rolledBack = true;

                        if (pController->IsLiveMode())
                        {
                            bool          needsRebuild = pController->LiveModeRebuildRequired();
                            Application * pApp         = pContext->m_pApp;

                            pController->CancelLiveMode();

                            if (pApp && needsRebuild)
                            {
                                pApp->ApplyDisplayModeChange();
                            }
                        }
                        else
                        {
                            pController->CancelChanges();
                        }

                        // See PSN_APPLY: no destroy from here.
                    }
                    // Explicitly allow the cancel to proceed.  PSN_RESET's
                    // return value is conveyed via DWLP_MSGRESULT: FALSE
                    // allows the close, TRUE prevents it.  Without this
                    // explicit clear, DWLP_MSGRESULT may carry over a
                    // non-zero value from a prior notification and silently
                    // veto Cancel / X / Esc.
                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, FALSE);
                    result = TRUE;
                    break;
            }
            break;
        }

        case WM_TIMER:
            if (wParam == kInfoTipDismissTimerId)
            {
                DismissInfoTip (hDlg);
                result = TRUE;
            }
            else if (wParam == kColorCycleTimerId)
            {
                // Re-invalidate the swatch; the paint code re-queries
                // GetColorRGB with a fresh elapsedTime so the swatch
                // animates at the same rate as the rain itself.
                InvalidateColorSwatch (hDlg);
                result = TRUE;
            }
            break;

        case WM_APP_RESET_RESYNC:
        {
            // Mini-phase 2.5 (cross-page Reset button): the frame's
            // SheetFrameSubclass already called controller->ResetToDefaults
            // (which pushed defaults through to ApplicationState for the
            // live-preview path), and is now broadcasting to every page so
            // each one re-syncs its own controls.  ResyncPageFromSettings
            // is page-agnostic — controls absent from this page are silent
            // no-ops via the null GetDlgItem.
            DialogContext * pContext = GetDialogContext (hDlg);


            if (pContext && pContext->m_controller)
            {
                ResyncPageFromSettings (hDlg, pContext->m_controller->GetSettings());
            }

            UpdateCycleTimerForCurrentScheme (hDlg);
            InvalidateColorSwatch            (hDlg);

            result = TRUE;
            break;
        }

        case WM_APP_OPEN_CUSTOM_COLOR_CHOOSER:
        {
            // T064/T065 (US5): the Color combo subclass posted this
            // (same-item re-click) OR the CBN_SELCHANGE handler did
            // (initial selection of Custom).  Both paths open the
            // modal chooser; on Cancel we revert the combo to the
            // previously selected scheme.  On OK, the controller is
            // already updated by OpenCustomColorChooser; reflect the
            // new selection index in our last-selection tracker.
            bool accepted = OpenCustomColorChooser (hDlg);


            if (accepted)
            {
                SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO,
                                     CB_SETCURSEL, kCustomColorComboIndex, 0);
                SetPropW (hDlg, kLastColorComboIndexProp,
                          reinterpret_cast<HANDLE> (static_cast<INT_PTR> (kCustomColorComboIndex + 1)));
            }
            else
            {
                int prior = static_cast<int> (reinterpret_cast<INT_PTR> (GetPropW (hDlg, kLastColorComboIndexProp))) - 1;


                if (prior < 0 || prior >= static_cast<int> (ARRAYSIZE (s_colorSchemeEntries)))
                {
                    prior = 0;
                }

                SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_SETCURSEL, prior, 0);
            }

            UpdateCycleTimerForCurrentScheme (hDlg);
            InvalidateColorSwatch            (hDlg);

            result = TRUE;
            break;
        }

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
//  v1.5 property-sheet host (T029, T031, T032, T033)
//
//  Builds the PROPSHEETHEADERW + 2 PROPSHEETPAGEW templates, installs the
//  1 Hz title-update timer in PSCB_INITIALIZED, and tears down the shared
//  DialogContext after the sheet closes.
//
////////////////////////////////////////////////////////////////////////////////

// Forward declaration — defined just before ShowConfigDialog so it can
// live alongside the SheetFrameSubclass that consumes its IDC_RESET_DEFAULTS
// WM_COMMAND.  Called from PropSheetCallback's PSCB_INITIALIZED branch.
static void CreateFrameResetButton (HWND hSheet);


static VOID CALLBACK PerfTitleTimerProc (HWND hSheet, UINT /*msg*/, UINT_PTR /*id*/, DWORD /*time*/)
{
    DialogContext * pContext = static_cast<DialogContext *> (GetPropW (hSheet, kSheetContextProp));


    if (!pContext)
    {
        return;
    }

    // FPS source — primary monitor's render context (FR-010).
    unsigned fps = 0;

    if (pContext->m_pApp)
    {
        MonitorRenderContext * pPrimary = pContext->m_pApp->GetPrimaryRenderContext();
        bool                   hasFps   = false;
        float                  fpsValue = 0.0f;

        if (pPrimary)
        {
            fpsValue = pPrimary->GetPublishedFps (hasFps);

            if (hasFps && fpsValue > 0.0f)
            {
                fps = static_cast<unsigned> (fpsValue + 0.5f);
            }
        }
    }

    // GPU% source — PDH counter shared with the debug overlay (FR-011).
    unsigned gpu     = 0;
    double   gpuLoad = QueryProcessGpuLoadPercent();

    if (gpuLoad >= 0.0)
    {
        gpu = static_cast<unsigned> (gpuLoad + 0.5);
    }

    WCHAR readout[64] = {};
    StringCchPrintfW (readout,
                      ARRAYSIZE (readout),
                      pContext->m_perfTitleFormat,
                      fps,
                      gpu);

    // Bail if value unchanged — avoids flicker on repaint of identical text.
    if (wcscmp (pContext->m_lastPerfTabTitle, readout) == 0)
    {
        return;
    }

    StringCchCopyW (pContext->m_lastPerfTabTitle,
                    ARRAYSIZE (pContext->m_lastPerfTabTitle),
                    readout);

    // Write to the bottom-right readout static on every page.  Pages may
    // not exist yet during the very first tick — GetDlgItem returns NULL
    // and SetWindowTextW is harmless on NULL? — no, it isn't; guard.
    for (int i = 0; i < 2; i++)
    {
        HWND hPage = PropSheet_IndexToHwnd (hSheet, i);


        if (hPage)
        {
            HWND hReadout = GetDlgItem (hPage, IDC_FPS_GPU_READOUT);


            if (hReadout)
            {
                SetWindowTextW (hReadout, readout);
            }
        }
    }
}





static int CALLBACK PropSheetCallback (HWND hSheet, UINT uMsg, LPARAM lParam)
{
    UNREFERENCED_PARAMETER (lParam);


    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            DialogContext * pContext = static_cast<DialogContext *> (GetPropW (hSheet, kSheetContextProp));

            if (!pContext)
            {
                break;
            }

            pContext->m_hSheet = hSheet;

            {
                LONG_PTR exStyle = GetWindowLongPtrW (hSheet, GWL_EXSTYLE);


                SetWindowLongPtrW (hSheet, GWL_EXSTYLE, exStyle & ~WS_EX_CONTEXTHELP);
                SetWindowPos (hSheet,
                              nullptr,
                              0,
                              0,
                              0,
                              0,
                              SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            }

            // Update Application's "current dialog" pointer to the sheet
            // frame so the main message loop's IsDialogMessage branch picks
            // up Tab/Enter routing correctly (T031, research.md R1).
            if (pContext->m_pApp)
            {
                pContext->m_pApp->SetConfigDialog (hSheet);
            }

            // Centring is deferred to WM_APP_REPOSITION_RESET (posted at
            // the bottom of PSCB_INITIALIZED).  PSCB_INITIALIZED runs
            // BEFORE comctl32 resizes the sheet to fit the pages, so
            // GetWindowRect here would return the small-template size and
            // mis-centre the dialog.  The posted message fires after
            // PropertySheetW returns to the message loop, by which point
            // the sheet has its real outer dimensions.

            // Load the format string once and cache on the context.
            LoadStringW (GetModuleHandleW (nullptr),
                         IDS_PERFTAB_TITLE_FORMAT,
                         pContext->m_perfTitleFormat,
                         ARRAYSIZE (pContext->m_perfTitleFormat));

            // 1 Hz timer; TimerProc form so we don't have to subclass the
            // comctl32-owned frame to catch WM_TIMER.  KillTimer is implicit
            // when the frame is destroyed.
            SetTimer (hSheet, kPerfTitleTimerId, 1000, PerfTitleTimerProc);

            // Fire one immediate tick so the title shows something other
            // than "Performance" before the first second elapses.
            PerfTitleTimerProc (hSheet, WM_TIMER, kPerfTitleTimerId, GetTickCount());

            // Mini-phase 2.5: create the frame-scope "Reset to defaults"
            // pushbutton (property sheets don't expose one natively).  The
            // SheetFrameSubclass intercepts its WM_COMMAND/BN_CLICKED and
            // broadcasts WM_APP_RESET_RESYNC to both pages.  Created here
            // (not in the trampoline) so the page HWNDs and the OK button
            // already exist for the layout math in CreateFrameResetButton.
            //
            // PSCB_INITIALIZED runs BEFORE the property sheet finishes
            // its internal resize-around-pages step — so the OK button
            // is still at its initial small-template position right now.
            // We create the Reset button anywhere here, then POST a
            // WM_APP_REPOSITION_RESET to ourselves so the SheetFrameSubclass
            // can reposition it AFTER PropertySheetW returns control to the
            // message loop (i.e., after the sheet has finalized its layout).
            CreateFrameResetButton (hSheet);
            PostMessageW (hSheet, WM_APP_REPOSITION_RESET, 0, 0);
            break;
        }
    }

    return 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  BuildPropSheet — populate PROPSHEETPAGEW[2] and PROPSHEETHEADERW pointing
//  at the Visuals + Performance templates and at PageDlgProc.
//
////////////////////////////////////////////////////////////////////////////////

static void BuildPropSheet (HINSTANCE         hInstance,
                             HWND              parentHwnd,
                             DialogContext   * pContext,
                             bool              modeless,
                             PROPSHEETPAGEW    psPages[2],
                             PROPSHEETHEADERW & psHeader)
{
    psPages[0]              = {};
    psPages[0].dwSize       = sizeof (PROPSHEETPAGEW);
    psPages[0].dwFlags      = PSP_USETITLE | PSP_PREMATURE;
    psPages[0].hInstance    = hInstance;
    psPages[0].pszTemplate  = MAKEINTRESOURCEW (IDD_VISUALS_PAGE);
    psPages[0].pszTitle     = MAKEINTRESOURCEW (IDS_VISUALS_TAB_TITLE);
    psPages[0].pfnDlgProc   = PageDlgProc;
    psPages[0].lParam       = reinterpret_cast<LPARAM> (pContext);

    psPages[1]              = {};
    psPages[1].dwSize       = sizeof (PROPSHEETPAGEW);
    psPages[1].dwFlags      = PSP_USETITLE | PSP_PREMATURE;
    psPages[1].hInstance    = hInstance;
    psPages[1].pszTemplate  = MAKEINTRESOURCEW (IDD_PERFORMANCE_PAGE);
    psPages[1].pszTitle     = MAKEINTRESOURCEW (IDS_PERFORMANCE_TAB_TITLE_INITIAL);
    psPages[1].pfnDlgProc   = PageDlgProc;
    psPages[1].lParam       = reinterpret_cast<LPARAM> (pContext);

    pContext->m_isModeless  = modeless;

    psHeader                = {};
    psHeader.dwSize         = sizeof (PROPSHEETHEADERW);
    psHeader.dwFlags        = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_PROPTITLE | PSH_USECALLBACK
                              | (modeless ? PSH_MODELESS : 0u);
    psHeader.hwndParent     = parentHwnd;
    psHeader.hInstance      = hInstance;
    psHeader.pszCaption     = L"MatrixRain configuration";
    psHeader.nPages         = 2;
    psHeader.nStartPage     = 0;
    psHeader.ppsp           = psPages;
    psHeader.pfnCallback    = PropSheetCallback;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Sheet-frame subclass (mini-phase 2.5 + v1.5 lifetime cleanup)
//
//  Installed by both modal (`ShowConfigDialog`) and modeless
//  (`CreateConfigDialog`) trampolines on the first `PSCB_INITIALIZED`
//  tick.  Two responsibilities:
//
//    1. Frame-scope Reset button (`IDC_RESET_DEFAULTS`): intercept
//       `WM_COMMAND`, call `controller->ResetToDefaults()` (the controller
//       pushes through to `ApplicationState` for the live preview), then
//       broadcast `WM_APP_RESET_RESYNC` to every page so each tab refreshes
//       its own controls.  Also re-mirrors the glow-enabled state through
//       `ApplyGlowEnabledUI` since the default value re-enables the
//       greyed-out glow trios.
//
//    2. `WM_DESTROY` cleanup: clear `m_pApp->SetConfigDialog(nullptr)`,
//       post-quit in screensaver-CPL mode, delete the info-tip font, and
//       (modeless-only) `delete pContext` since the modeless path heap-
//       allocates the `DialogContext`.
//
////////////////////////////////////////////////////////////////////////////////

namespace
{


constexpr UINT_PTR kOkButtonSubclassId = 0xC0C00C2Eu;




static void RepositionFrameResetButton (HWND hSheet)
{
    HWND hReset = GetDlgItem (hSheet, IDC_RESET_DEFAULTS);
    HWND hOk    = GetDlgItem (hSheet, IDOK);


    if (hReset && hOk)
    {
        RECT  okScreenRect  = {};
        RECT  resetWinRect  = {};
        HWND  hTab          = nullptr;
        POINT okTopLeft     = {};
        int   okClientY     = 0;
        int   okHeight      = 0;
        int   leftEdgeX     = 15;     // safe default if tab control missing
        int   resetWidth    = 0;


        GetWindowRect (hOk, &okScreenRect);

        okTopLeft.x = okScreenRect.left;
        okTopLeft.y = okScreenRect.top;
        ScreenToClient (hSheet, &okTopLeft);

        GetWindowRect (hReset, &resetWinRect);

        okClientY  = okTopLeft.y;
        okHeight   = okScreenRect.bottom - okScreenRect.top;
        resetWidth = resetWinRect.right - resetWinRect.left;

        // Anchor Reset's LEFT edge to the tab control's LEFT edge so it
        // sits in the same content column as everything else.  Mirroring
        // OK's right margin (the prior approach) put Reset way too far
        // from the dialog's left edge because OK is anchored well inboard
        // of the right edge.
        hTab = PropSheet_GetTabControl (hSheet);

        if (hTab)
        {
            RECT  tabScreenRect = {};
            POINT tabTopLeft    = {};

            GetWindowRect (hTab, &tabScreenRect);
            tabTopLeft.x = tabScreenRect.left;
            tabTopLeft.y = tabScreenRect.top;
            ScreenToClient (hSheet, &tabTopLeft);
            leftEdgeX    = tabTopLeft.x;
        }

        SetWindowPos (hReset,
                      nullptr,
                      leftEdgeX,
                      okClientY,
                      resetWidth,
                      okHeight,
                      SWP_NOZORDER | SWP_NOACTIVATE);
    }
}




LRESULT CALLBACK OkButtonSubclass (HWND     hOk,
                                    UINT     msg,
                                    WPARAM   wParam,
                                    LPARAM   lParam,
                                    UINT_PTR uIdSubclass,
                                    DWORD_PTR /*dwRefData*/)
{
    if (msg == WM_WINDOWPOSCHANGED)
    {
        RepositionFrameResetButton (GetParent (hOk));
    }
    else if (msg == WM_NCDESTROY)
    {
        RemoveWindowSubclass (hOk, OkButtonSubclass, uIdSubclass);
    }

    return DefSubclassProc (hOk, msg, wParam, lParam);
}




LRESULT CALLBACK SheetFrameSubclass (HWND     hSheet,
                                      UINT     msg,
                                      WPARAM   wParam,
                                      LPARAM   lParam,
                                      UINT_PTR uIdSubclass,
                                      DWORD_PTR /*dwRefData*/)
{
    // Mini-phase 2.5: reposition the frame-scope Reset button to its
    // final left-aligned footer position.  Must happen AFTER property
    // sheet has finished sizing itself around the pages — PSCB_INITIALIZED
    // (where the button is created) fires while OK/Cancel are still at
    // their initial small-template positions, so any positioning done
    // there strands Reset in the middle of the dialog.  We use a posted
    // custom message (WM_APP_REPOSITION_RESET) dispatched from the
    // message loop AFTER PropertySheetW returns control — by then the
    // sheet has finalized its layout and OK is in its real footer slot.
    if (msg == WM_APP_REPOSITION_RESET)
    {
        RepositionFrameResetButton (hSheet);

        // Centre the sheet now that comctl32 has finalized its outer
        // dimensions to fit the pages.  Centering at PSCB_INITIALIZED
        // uses pre-resize geometry and lands the dialog off-centre.
        {
            DialogContext * pContext = static_cast<DialogContext *> (GetPropW (hSheet, kSheetContextProp));
            RECT            sheetRect  = {};
            RECT            centerRect = {};
            HWND            appHwnd    = (pContext && pContext->m_pApp) ? pContext->m_pApp->GetMainWindowHwnd() : nullptr;


            GetWindowRect (hSheet, &sheetRect);

            int sheetW = sheetRect.right  - sheetRect.left;
            int sheetH = sheetRect.bottom - sheetRect.top;

            if (appHwnd && IsWindow (appHwnd))
            {
                GetWindowRect (appHwnd, &centerRect);
            }
            else
            {
                centerRect.left   = 0;
                centerRect.top    = 0;
                centerRect.right  = GetSystemMetrics (SM_CXSCREEN);
                centerRect.bottom = GetSystemMetrics (SM_CYSCREEN);
            }

            int x = (centerRect.left + centerRect.right  - sheetW) / 2;
            int y = (centerRect.top  + centerRect.bottom - sheetH) / 2;

            SetWindowPos (hSheet, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    // Mini-phase 2.5: frame-scope Reset button.  Pages don't see this
    // WM_COMMAND because the button is parented to the sheet, not a page.
    if (msg == WM_COMMAND && LOWORD (wParam) == IDC_RESET_DEFAULTS && HIWORD (wParam) == BN_CLICKED)
    {
        DialogContext * pContext = static_cast<DialogContext *> (GetPropW (hSheet, kSheetContextProp));


        if (pContext && pContext->m_controller)
        {
            // Reset settings; controller pushes through to ApplicationState
            // in live mode so the live preview snaps back instantly.
            pContext->m_controller->ResetToDefaults();

            const ScreenSaverSettings & defaults = pContext->m_controller->GetSettings();

            // Re-mirror glow-enabled state across both pages' EnableWindow
            // flags (the default is glow ON, so any greyed-out trios from
            // a prior glow-off state need to re-enable).
            ApplyGlowEnabledUI (hSheet, defaults.m_glowEnabled);

            // Broadcast to every page so each one re-syncs its own
            // controls from the freshly-reset settings.  PSP_PREMATURE
            // guarantees both page HWNDs already exist.
            for (int i = 0; i < 2; i++)
            {
                HWND hPage = PropSheet_IndexToHwnd (hSheet, i);


                if (hPage)
                {
                    SendMessageW (hPage, WM_APP_RESET_RESYNC, 0, 0);
                }
            }
        }

        return 0;
    }

    if (msg == WM_DESTROY)
    {
        DialogContext * pContext = static_cast<DialogContext *> (GetPropW (hSheet, kSheetContextProp));


        if (pContext)
        {
            // Tear down per-sheet resources before the frame is gone.
            if (pContext->m_pApp)
            {
                pContext->m_pApp->SetConfigDialog (nullptr);

                if (pContext->m_pApp->GetScreenSaverMode() == ScreenSaverMode::SettingsDialog)
                {
                    PostQuitMessage (0);
                }
            }

            if (pContext->m_hInfoTipFont)
            {
                DeleteObject (pContext->m_hInfoTipFont);
                pContext->m_hInfoTipFont = nullptr;
            }

            RemovePropW (hSheet, kSheetContextProp);

            if (pContext->m_ownsContextMemory)
            {
                delete pContext;
            }
        }
    }
    else if (msg == WM_NCDESTROY)
    {
        RemoveWindowSubclass (hSheet, SheetFrameSubclass, uIdSubclass);
    }

    return DefSubclassProc (hSheet, msg, wParam, lParam);
}


constexpr UINT_PTR kSheetFrameSubclassId = 0xC0C00C2Du;


}  // namespace




////////////////////////////////////////////////////////////////////////////////
//
//  CreateFrameResetButton — mini-phase 2.5 helper.  Property sheets don't
//  expose a native Reset button, so we create one in PSCB_INITIALIZED and
//  parent it to the sheet frame.  Positioned at the footer-left edge,
//  mirroring the right margin of the OK button.  Font matches OK's font
//  so the button renders consistently across the v6 comctl32 themes.
//
////////////////////////////////////////////////////////////////////////////////

static void CreateFrameResetButton (HWND hSheet)
{
    HWND      hOk          = GetDlgItem (hSheet, IDOK);
    HWND      hReset       = nullptr;
    HFONT     hFont        = nullptr;
    HINSTANCE hInst        = nullptr;
    RECT      okScreenRect = {};
    RECT      sheetClient  = {};
    POINT     okTopLeft    = {};
    int       okWidth      = 0;
    int       okHeight     = 0;
    int       okClientY    = 0;
    int       okClientX    = 0;
    int       rightMargin  = 0;
    int       resetWidth   = 0;



    if (!hOk)
    {
        return;
    }


    GetWindowRect  (hOk,    &okScreenRect);
    GetClientRect  (hSheet, &sheetClient);

    okTopLeft.x = okScreenRect.left;
    okTopLeft.y = okScreenRect.top;
    ScreenToClient (hSheet, &okTopLeft);

    okClientX   = okTopLeft.x;
    okClientY   = okTopLeft.y;
    okWidth     = okScreenRect.right  - okScreenRect.left;
    okHeight    = okScreenRect.bottom - okScreenRect.top;
    rightMargin = sheetClient.right - (okClientX + okWidth);

    // "Reset to defaults" is wider than "OK"; size it to ~1.7x the OK width
    // so the label fits at the default font without crowding (the v6
    // themed button has its own internal padding so we don't need extra).
    resetWidth  = static_cast<int> (okWidth * 1.7);

    hInst       = reinterpret_cast<HINSTANCE> (GetWindowLongPtrW (hSheet, GWLP_HINSTANCE));

    hReset = CreateWindowExW (0,
                              L"BUTTON",
                              L"Reset to defaults",
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                              rightMargin,
                              okClientY,
                              resetWidth,
                              okHeight,
                              hSheet,
                              reinterpret_cast<HMENU> (static_cast<INT_PTR> (IDC_RESET_DEFAULTS)),
                              hInst,
                              nullptr);

    if (hReset)
    {
        hFont = reinterpret_cast<HFONT> (SendMessageW (hOk, WM_GETFONT, 0, 0));

        if (hFont)
        {
            SendMessageW (hReset, WM_SETFONT, reinterpret_cast<WPARAM> (hFont), TRUE);
        }

        SetWindowSubclass (hOk, OkButtonSubclass, kOkButtonSubclassId, 0);
        RepositionFrameResetButton (hSheet);
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  ShowConfigDialog (modal, Control Panel /c:<HWND>)
//
////////////////////////////////////////////////////////////////////////////////

int ShowConfigDialog (HINSTANCE hInstance, const ScreenSaverModeContext & context)
{
    HRESULT          hr         = S_OK;
    DialogContext    dlgContext;
    PROPSHEETPAGEW   psPages[2] = {};
    PROPSHEETHEADERW psHeader   = {};
    HWND             parentHwnd = context.m_previewParentHwnd;
    INT_PTR          result     = -1;
    


    dlgContext.m_controller = std::make_unique<ConfigDialogController> (dlgContext.m_settingsProvider);
    dlgContext.m_isScreenSaverCPL = true;
    hr = dlgContext.m_controller->Initialize();
    CHRA (hr);

    BuildPropSheet (hInstance, parentHwnd, &dlgContext, /*modeless=*/false, psPages, psHeader);

    // Stash the context on a "hint" property of a temp hidden window?  No —
    // PSCB_INITIALIZED runs on the sheet hwnd which we don't have ahead of
    // time.  Workaround: a static thread_local fallback used inside the
    // callback as a bootstrap (cleared on entry).  Modal path is single-
    // threaded so this is safe.
    static thread_local DialogContext * tls_pendingContext = nullptr;
    tls_pendingContext = &dlgContext;

    // Wrapper callback that installs the SetProp + sheet subclass on first
    // call, then delegates to PropSheetCallback.  We use a lambda-free
    // static here so it's a plain C callback.  The subclass is what makes
    // the frame-scope Reset button work (mini-phase 2.5) and also handles
    // WM_DESTROY cleanup of the sheet-context property.
    struct CbHelper
    {
        static int CALLBACK Trampoline (HWND hSheet, UINT uMsg, LPARAM lParam)
        {
            if (uMsg == PSCB_INITIALIZED && tls_pendingContext)
            {
                SetPropW          (hSheet, kSheetContextProp, tls_pendingContext);
                SetWindowSubclass (hSheet, SheetFrameSubclass, kSheetFrameSubclassId, 0);
                tls_pendingContext = nullptr;
            }
            return PropSheetCallback (hSheet, uMsg, lParam);
        }
    };
    psHeader.pfnCallback = CbHelper::Trampoline;

    result = PropertySheetW (&psHeader);

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

    // PropertySheetW returns ID_PSREBOOT, ID_PSRESTARTWINDOWS, 1 (OK), or 0
    // (Cancel) in modal mode.  Map to IDOK / IDCANCEL for the caller.
    return (result > 0) ? IDOK : IDCANCEL;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CreateConfigDialog (modeless, live overlay)
//
//  Returns the property-sheet frame HWND via phDlg; lifetime of the
//  DialogContext is managed by DestroySheetContext when the frame's
//  WM_DESTROY fires (we observe it via a subclass installed in
//  PSCB_INITIALIZED).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CreateConfigDialog (HINSTANCE          hInstance,
                             HWND               parentHwnd,
                             Application      * pApplication,
                             ApplicationState * pAppState,
                             HWND             * phDlg)
{
    HRESULT          hr         = S_OK;
    HWND             hSheet     = nullptr;
    PROPSHEETPAGEW   psPages[2] = {};
    PROPSHEETHEADERW psHeader   = {};
    auto             context    = std::make_unique<DialogContext>();


    
    context->m_controller = std::make_unique<ConfigDialogController> (pApplication->GetSettingsProvider());
    hr = context->m_controller->Initialize();
    CHRA (hr);

    hr = context->m_controller->InitializeLiveMode (pAppState);
    CHRA (hr);

    context->m_pApp              = pApplication;
    context->m_ownsContextMemory = true;

    {
        DialogContext * pCtx = context.get();

        BuildPropSheet (hInstance, parentHwnd, pCtx, /*modeless=*/true, psPages, psHeader);

        // Modeless trampoline (mirrors the modal version): bootstrap the
        // SetPropW(kSheetContextProp) on first PSCB_INITIALIZED, install the
        // frame subclass for WM_DESTROY cleanup, then delegate.
        static thread_local DialogContext * tls_pendingContext = nullptr;
        tls_pendingContext = pCtx;

        struct CbHelper
        {
            static int CALLBACK Trampoline (HWND hSheet, UINT uMsg, LPARAM lParam)
            {
                if (uMsg == PSCB_INITIALIZED && tls_pendingContext)
                {
                    SetPropW (hSheet, kSheetContextProp, tls_pendingContext);
                    SetWindowSubclass (hSheet, SheetFrameSubclass, kSheetFrameSubclassId, 0);
                    tls_pendingContext = nullptr;
                }
                return PropSheetCallback (hSheet, uMsg, lParam);
            }
        };
        psHeader.pfnCallback = CbHelper::Trampoline;
    }

    hSheet = reinterpret_cast<HWND> (PropertySheetW (&psHeader));
    CWRA (hSheet);

    context.release();   // ownership transferred to the sheet subclass

    ShowWindow (hSheet, SW_SHOW);

Error:
    if (FAILED (hr))
    {
        MessageBoxW (nullptr, L"Failed to create live overlay configuration dialog.", L"Error", MB_OK | MB_ICONERROR);
        hSheet = nullptr;
    }

    *phDlg = hSheet;

    return hr;
}
