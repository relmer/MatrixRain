#include "pch.h"

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
    // DXGI adapter description (NOT the "(default)"-suffixed display label)
    // so OnGpuChange can map a selection back to the persistence string.
    // Index 0 is the synthetic "<system default>" sentinel and corresponds
    // to an empty m_gpuAdapter setting.
    std::vector<std::wstring>                 m_gpuAdapterDescriptions;
};





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
//  InitializeSlider
//
////////////////////////////////////////////////////////////////////////////////

static void InitializeSlider (HWND hDlg, int sliderId, int labelId, int minValue, int maxValue, int currentValue)
{
    SendDlgItemMessageW (hDlg, sliderId, TBM_SETRANGE, TRUE, MAKELPARAM (minValue, maxValue));
    SendDlgItemMessageW (hDlg, sliderId, TBM_SETPOS, TRUE, currentValue);
    SetDlgItemTextW     (hDlg, labelId, std::format (L"{}%", currentValue).c_str());
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
//  Enumerate the system's rendering adapters via WindowsAdapterProvider,
//  prefix a synthetic "<system default>" entry so the user can revert to
//  the OS-chosen GPU at any time, and select whichever entry corresponds to
//  the persisted m_gpuAdapter description.
//
////////////////////////////////////////////////////////////////////////////////

static void InitializeGpuCombo (HWND hDlg, DialogContext * pContext, const std::wstring & currentDescription)
{
    WindowsAdapterProvider   provider;
    std::vector<AdapterInfo> adapters = provider.EnumerateAdapters();
    int                      selected = 0;


    // Synthetic first entry: "<system default>" maps to an empty
    // persisted description (= use whatever the OS picks).
    pContext->m_gpuAdapterDescriptions.clear();
    pContext->m_gpuAdapterDescriptions.push_back (L"");
    SendDlgItemMessageW (hDlg, IDC_GPU_COMBO, CB_ADDSTRING, 0, (LPARAM) L"<system default>");

    for (const AdapterInfo & adapter : adapters)
    {
        std::wstring label = FormatAdapterLabel (adapter);

        SendDlgItemMessageW (hDlg, IDC_GPU_COMBO, CB_ADDSTRING, 0, (LPARAM) label.c_str());

        pContext->m_gpuAdapterDescriptions.push_back (adapter.m_description);

        if (!currentDescription.empty() && adapter.m_description == currentDescription)
        {
            selected = static_cast<int> (pContext->m_gpuAdapterDescriptions.size()) - 1;
        }
    }


    SendDlgItemMessageW (hDlg, IDC_GPU_COMBO, CB_SETCURSEL, selected, 0);
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
            SetDlgItemTextW (hDlg, IDC_DENSITY_LABEL, std::format (L"{}%", pos).c_str());
            break;
            
        case IDC_ANIMSPEED_SLIDER:
            pController->UpdateAnimationSpeed (pos);
            SetDlgItemTextW (hDlg, IDC_ANIMSPEED_LABEL, std::format (L"{}%", pos).c_str());
            break;
            
        case IDC_GLOWINTENSITY_SLIDER:
            pController->UpdateGlowIntensity (pos);
            SetDlgItemTextW (hDlg, IDC_GLOWINTENSITY_LABEL, std::format (L"{}%", pos).c_str());
            break;
            
        case IDC_GLOWSIZE_SLIDER:
            pController->UpdateGlowSize (pos);
            SetDlgItemTextW (hDlg, IDC_GLOWSIZE_LABEL, std::format (L"{}%", pos).c_str());
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
        pController->UpdateDensity          (pDefaults->m_densityPercent);
        pController->UpdateAnimationSpeed   (pDefaults->m_animationSpeedPercent);
        pController->UpdateGlowIntensity    (pDefaults->m_glowIntensityPercent);
        pController->UpdateGlowSize         (pDefaults->m_glowSizePercent);
        pController->UpdateColorScheme      (pDefaults->m_colorSchemeKey.c_str());
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
        Application * pApp = GetApplicationFromDialog (hDlg);

        pController->CancelLiveMode();

        // The snapshot restore in CancelLiveMode reverts settings fields
        // (including m_multiMonitorEnabled) but does not by itself trigger
        // a context rebuild.  Post one explicitly so the live multimon /
        // display-mode preview reverts visually (FR-031b).
        if (pApp)
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
