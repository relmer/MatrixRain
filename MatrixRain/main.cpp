#include "pch.h"

#include "..\MatrixRainCore\Application.h"
#include "..\MatrixRainCore\ConfigDialogController.h"
#include "..\MatrixRainCore\ScreenSaverModeParser.h"
#include "resource.h"





static ConfigDialogController * g_pController = nullptr;





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





////////////////////////////////////////////////////////////////////////////////
//
//  InitializeColorSchemeCombo
//
////////////////////////////////////////////////////////////////////////////////

static void InitializeColorSchemeCombo (HWND hDlg, const std::wstring & currentScheme)
{
    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_ADDSTRING, 0, (LPARAM)L"green");
    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_ADDSTRING, 0, (LPARAM)L"blue");
    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_ADDSTRING, 0, (LPARAM)L"red");
    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_ADDSTRING, 0, (LPARAM)L"amber");
    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_ADDSTRING, 0, (LPARAM)L"cycle");
    
    int index = 0;
    
    if      (currentScheme == L"green") index = 0;
    else if (currentScheme == L"blue")  index = 1;
    else if (currentScheme == L"red")   index = 2;
    else if (currentScheme == L"amber") index = 3;
    else if (currentScheme == L"cycle") index = 4;
    
    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_SETCURSEL, index, 0);
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnInitDialog
//
////////////////////////////////////////////////////////////////////////////////

static BOOL OnInitDialog (HWND hDlg)
{
    if (g_pController == nullptr)
    {
        return FALSE;
    }

    const ScreenSaverSettings & settings = g_pController->GetSettings();
    
    InitializeSlider (hDlg, IDC_DENSITY_SLIDER, IDC_DENSITY_LABEL, 
                      ScreenSaverSettings::MIN_DENSITY_PERCENT, ScreenSaverSettings::MAX_DENSITY_PERCENT, 
                      settings.m_densityPercent);
    
    InitializeSlider (hDlg, IDC_ANIMSPEED_SLIDER, IDC_ANIMSPEED_LABEL,
                      ScreenSaverSettings::MIN_ANIMATION_SPEED_PERCENT, ScreenSaverSettings::MAX_ANIMATION_SPEED_PERCENT,
                      settings.m_animationSpeedPercent);
    
    InitializeSlider (hDlg, IDC_GLOWINTENSITY_SLIDER, IDC_GLOWINTENSITY_LABEL,
                      ScreenSaverSettings::MIN_GLOW_INTENSITY_PERCENT, ScreenSaverSettings::MAX_GLOW_INTENSITY_PERCENT,
                      settings.m_glowIntensityPercent);
    
    InitializeSlider (hDlg, IDC_GLOWSIZE_SLIDER, IDC_GLOWSIZE_LABEL,
                      ScreenSaverSettings::MIN_GLOW_SIZE_PERCENT, ScreenSaverSettings::MAX_GLOW_SIZE_PERCENT,
                      settings.m_glowSizePercent);
    
    InitializeColorSchemeCombo (hDlg, settings.m_colorSchemeKey);
    
    CheckDlgButton (hDlg, IDC_STARTFULLSCREEN_CHECK, settings.m_startFullscreen ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWDEBUG_CHECK, settings.m_showDebugStats ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWFADETIMERS_CHECK, settings.m_showFadeTimers ? BST_CHECKED : BST_UNCHECKED);
    
    return TRUE;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnHScroll
//
////////////////////////////////////////////////////////////////////////////////

static BOOL OnHScroll (HWND hDlg, LPARAM lParam)
{
    if (g_pController == nullptr)
    {
        return FALSE;
    }

    HWND hSlider = (HWND)lParam;
    int  ctrlId  = GetDlgCtrlID (hSlider);
    int  pos     = (int)SendMessageW (hSlider, TBM_GETPOS, 0, 0);
    
    switch (ctrlId)
    {
        case IDC_DENSITY_SLIDER:
            g_pController->UpdateDensity (pos);
            SetDlgItemTextW (hDlg, IDC_DENSITY_LABEL, std::format (L"{}%", pos).c_str());
            break;
            
        case IDC_ANIMSPEED_SLIDER:
            g_pController->UpdateAnimationSpeed (pos);
            SetDlgItemTextW (hDlg, IDC_ANIMSPEED_LABEL, std::format (L"{}%", pos).c_str());
            break;
            
        case IDC_GLOWINTENSITY_SLIDER:
            g_pController->UpdateGlowIntensity (pos);
            SetDlgItemTextW (hDlg, IDC_GLOWINTENSITY_LABEL, std::format (L"{}%", pos).c_str());
            break;
            
        case IDC_GLOWSIZE_SLIDER:
            g_pController->UpdateGlowSize (pos);
            SetDlgItemTextW (hDlg, IDC_GLOWSIZE_LABEL, std::format (L"{}%", pos).c_str());
            break;
    }
    
    return TRUE;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnColorSchemeChange
//
////////////////////////////////////////////////////////////////////////////////

static void OnColorSchemeChange (HWND hDlg)
{
    int   index = (int)SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_GETCURSEL, 0, 0);
    WCHAR scheme[32];
    
    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_GETLBTEXT, index, (LPARAM)scheme);
    g_pController->UpdateColorScheme (scheme);
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnResetButton
//
////////////////////////////////////////////////////////////////////////////////

static void OnResetButton (HWND hDlg)
{
    g_pController->ResetToDefaults();
    
    const ScreenSaverSettings & defaults = g_pController->GetSettings();
    
    SendDlgItemMessageW (hDlg, IDC_DENSITY_SLIDER, TBM_SETPOS, TRUE, defaults.m_densityPercent);
    SetDlgItemTextW     (hDlg, IDC_DENSITY_LABEL, std::format (L"{}%", defaults.m_densityPercent).c_str());
    
    SendDlgItemMessageW (hDlg, IDC_ANIMSPEED_SLIDER, TBM_SETPOS, TRUE, defaults.m_animationSpeedPercent);
    SetDlgItemTextW     (hDlg, IDC_ANIMSPEED_LABEL, std::format (L"{}%", defaults.m_animationSpeedPercent).c_str());
    
    SendDlgItemMessageW (hDlg, IDC_GLOWINTENSITY_SLIDER, TBM_SETPOS, TRUE, defaults.m_glowIntensityPercent);
    SetDlgItemTextW     (hDlg, IDC_GLOWINTENSITY_LABEL, std::format (L"{}%", defaults.m_glowIntensityPercent).c_str());
    
    SendDlgItemMessageW (hDlg, IDC_GLOWSIZE_SLIDER, TBM_SETPOS, TRUE, defaults.m_glowSizePercent);
    SetDlgItemTextW     (hDlg, IDC_GLOWSIZE_LABEL, std::format (L"{}%", defaults.m_glowSizePercent).c_str());
    
    CheckDlgButton (hDlg, IDC_STARTFULLSCREEN_CHECK, defaults.m_startFullscreen ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWDEBUG_CHECK, defaults.m_showDebugStats ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWFADETIMERS_CHECK, defaults.m_showFadeTimers ? BST_CHECKED : BST_UNCHECKED);
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnOK
//
////////////////////////////////////////////////////////////////////////////////

static BOOL OnOK (HWND hDlg)
{
    HRESULT hr = g_pController->ApplyChanges();
    
    if (FAILED (hr))
    {
        MessageBoxW (hDlg, L"Failed to save settings to registry.", L"Error", MB_OK | MB_ICONERROR);
        return TRUE;
    }
    
    EndDialog (hDlg, IDOK);
    return TRUE;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnCancel
//
////////////////////////////////////////////////////////////////////////////////

static BOOL OnCancel (HWND hDlg)
{
    g_pController->CancelChanges();
    EndDialog (hDlg, IDCANCEL);
    return TRUE;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnCommand
//
////////////////////////////////////////////////////////////////////////////////

static BOOL OnCommand (HWND hDlg, WPARAM wParam)
{
    if (g_pController == nullptr)
    {
        return FALSE;
    }

    switch (LOWORD (wParam))
    {
        case IDC_COLORSCHEME_COMBO:
            if (HIWORD (wParam) == CBN_SELCHANGE)
            {
                OnColorSchemeChange (hDlg);
            }
            break;
            
        case IDC_STARTFULLSCREEN_CHECK:
            g_pController->UpdateStartFullscreen (IsDlgButtonChecked (hDlg, IDC_STARTFULLSCREEN_CHECK) == BST_CHECKED);
            break;
            
        case IDC_SHOWDEBUG_CHECK:
            g_pController->UpdateShowDebugStats (IsDlgButtonChecked (hDlg, IDC_SHOWDEBUG_CHECK) == BST_CHECKED);
            break;
            
        case IDC_SHOWFADETIMERS_CHECK:
            g_pController->UpdateShowFadeTimers (IsDlgButtonChecked (hDlg, IDC_SHOWFADETIMERS_CHECK) == BST_CHECKED);
            break;
            
        case IDC_RESET_BUTTON:
            OnResetButton (hDlg);
            break;
            
        case IDOK:
            return OnOK (hDlg);
            
        case IDCANCEL:
            return OnCancel (hDlg);
    }
    
    return FALSE;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogProc
//
//  Dialog procedure for IDD_MATRIXRAIN_SAVER_CONFIG.
//
////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK ConfigDialogProc (HWND   hDlg,
                                    UINT   message,
                                    WPARAM wParam,
                                    LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            return OnInitDialog (hDlg);
        
        case WM_HSCROLL:
            return OnHScroll (hDlg, lParam);
        
        case WM_COMMAND:
            return OnCommand (hDlg, wParam);
    }
    
    return FALSE;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ShowConfigDialog
//
//  Display configuration dialog for screensaver settings.
//  Modal mode (Control Panel): HWND provided via /c:<HWND>
//
////////////////////////////////////////////////////////////////////////////////

static int ShowConfigDialog (HINSTANCE hInstance, const ScreenSaverModeContext & context)
{
    HRESULT                hr           = S_OK;
    ConfigDialogController controller;
    int                    result       = 0;
    HWND                   parentHwnd   = context.m_previewParentHwnd;
    


    hr = controller.Initialize();
    CHRA (hr);

    g_pController = &controller;
    result        = (int)DialogBoxW (hInstance, MAKEINTRESOURCEW (IDD_MATRIXRAIN_SAVER_CONFIG), parentHwnd, ConfigDialogProc);
    g_pController = nullptr;

Error:
    if (FAILED (hr))
    {
        MessageBoxW (nullptr, L"Failed to initialize configuration dialog.", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    return result;
}





int APIENTRY wWinMain(_In_     HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_     LPWSTR    lpCmdLine,
                      _In_     int       nCmdShow)
{
    UNREFERENCED_PARAMETER (hPrevInstance);
    
    Application            app;
    HRESULT                hr      = S_OK;
    int                    retval  = 0;
    ScreenSaverModeContext context;



    // Set DPI awareness to per-monitor V2 for consistent physical pixel measurements
    SetProcessDpiAwarenessContext (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Parse command-line arguments
    hr = ParseCommandLine (lpCmdLine, context);
    CHR (hr);

    // Handle configuration mode with HWND (modal Control Panel dialog)
    if (context.m_mode == ScreenSaverMode::SettingsDialog && context.m_previewParentHwnd != nullptr)
    {
        return ShowConfigDialog (hInstance, context);
    }

    // For live overlay mode (/c without HWND) or normal operation, initialize the application
    hr = app.Initialize (hInstance, nCmdShow, &context);
    CHR (hr);
    
    // If this was a live overlay config request, show the dialog now over our window
    if (context.m_mode == ScreenSaverMode::SettingsDialog)
    {
        // TODO: Get app window HWND and ApplicationState pointer from app
        // TODO: Pass them to ShowConfigDialog for live overlay mode
        // For now, show error
        MessageBoxW (nullptr,
                     L"Live overlay configuration mode not yet fully implemented.\n\n"
                     L"Use Control Panel → Screen Saver Settings instead.",
                     L"MatrixRain Configuration",
                     MB_OK | MB_ICONINFORMATION);
        return 0;
    }
    
    retval = app.Run();


Error:
    if (FAILED (hr))
    {
        LPCWSTR errorMsg = L"Failed to initialize MatrixRain";

        if (context.m_errorMessage.empty() == false)
        {
            errorMsg = context.m_errorMessage.c_str();
        }

        MessageBoxW (nullptr, errorMsg, L"Error", MB_OK | MB_ICONERROR);
        retval = -1;
    }

    return retval;
}
