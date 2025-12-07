#include "pch.h"

#include "ConfigDialog.h"
#include "..\MatrixRainCore\Application.h"
#include "..\MatrixRainCore\ApplicationState.h"
#include "..\MatrixRainCore\ConfigDialogController.h"
#include "..\MatrixRainCore\ScreenSaverModeParser.h"
#include "resource.h"





struct DialogContext
{
    std::unique_ptr<ConfigDialogController> m_controller;
    Application                           * m_pApp              = nullptr;
    bool                                    m_ownsContextMemory = false;
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

    // Center dialog on parent window or primary monitor
    GetWindowRect (hDlg, &dialogRect);
    dialogWidth  = dialogRect.right  - dialogRect.left;
    dialogHeight = dialogRect.bottom - dialogRect.top;

    if (parentHwnd)
    {
        GetWindowRect (parentHwnd, &centerRect);       
    }
    else
    {
        // No parent - center on primary monitor
        centerRect.left   = 0;
        centerRect.top    = 0;
        centerRect.right  = GetSystemMetrics (SM_CXSCREEN);
        centerRect.bottom = GetSystemMetrics (SM_CYSCREEN);
    }
    
    GetWindowRect (hDlg, &dialogRect);
    dialogWidth  = dialogRect.right  - dialogRect.left;
    dialogHeight = dialogRect.bottom - dialogRect.top;
    
    // Calculate centered position
    centerX = (centerRect.left + centerRect.right  - dialogWidth)  / 2;
    centerY = (centerRect.top  + centerRect.bottom - dialogHeight) / 2;
    
    dialogPos.x = centerX;
    dialogPos.y = centerY;
    
    SetWindowPos (hDlg, nullptr, dialogPos.x, dialogPos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

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
    
    CheckDlgButton (hDlg, IDC_STARTFULLSCREEN_CHECK, pSettings->m_startFullscreen ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWDEBUG_CHECK,       pSettings->m_showDebugStats  ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWFADETIMERS_CHECK,  pSettings->m_showFadeTimers  ? BST_CHECKED : BST_UNCHECKED);
    
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
    WCHAR                    scheme[32];



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    index = (int)SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_GETCURSEL, 0, 0);
    
    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_GETLBTEXT, index, (LPARAM)scheme);
    pController->UpdateColorScheme (scheme);

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnResetButton
//
////////////////////////////////////////////////////////////////////////////////

static void OnResetButton (HWND hDlg)
{
    HRESULT                     hr          = S_OK;
    ConfigDialogController    * pController = GetControllerFromDialog (hDlg);
    Application               * pApp        = GetApplicationFromDialog (hDlg);
    const ScreenSaverSettings * pDefaults   = nullptr;



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
    
    CheckDlgButton (hDlg, IDC_STARTFULLSCREEN_CHECK, pDefaults->m_startFullscreen ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWDEBUG_CHECK,       pDefaults->m_showDebugStats  ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWFADETIMERS_CHECK,  pDefaults->m_showFadeTimers  ? BST_CHECKED : BST_UNCHECKED);
    
    // If live overlay mode, immediately apply changes to running application
    if (pApp)
    {
        pApp->GetApplicationState()->OnDensityChanged       (pDefaults->m_densityPercent);
        pApp->GetApplicationState()->SetAnimationSpeed      (pDefaults->m_animationSpeedPercent);
        pApp->GetApplicationState()->SetGlowIntensity       (pDefaults->m_glowIntensityPercent);
        pApp->GetApplicationState()->SetGlowSize            (pDefaults->m_glowSizePercent);
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

    pController->CancelChanges();
    
    // For modeless dialogs, use DestroyWindow instead of EndDialog
    if (pController->IsLiveMode())
    {
        DestroyWindow (hDlg);
    }
    else
    {
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
            
        case IDC_STARTFULLSCREEN_CHECK:
            pController->UpdateStartFullscreen (IsDlgButtonChecked (hDlg, IDC_STARTFULLSCREEN_CHECK) == BST_CHECKED);
            break;
            
        case IDC_SHOWDEBUG_CHECK:
            pController->UpdateShowDebugStats (IsDlgButtonChecked (hDlg, IDC_SHOWDEBUG_CHECK) == BST_CHECKED);
            break;
            
        case IDC_SHOWFADETIMERS_CHECK:
            pController->UpdateShowFadeTimers (IsDlgButtonChecked (hDlg, IDC_SHOWFADETIMERS_CHECK) == BST_CHECKED);
            break;
            
        case IDC_RESET_BUTTON:
            OnResetButton (hDlg);
            break;
            
        case IDOK:
            fSuccess = OnOK (hDlg);
            goto Error;
            
        case IDCANCEL:
            fSuccess = OnCancel (hDlg);
            goto Error;
    }

Error:
    return fSuccess;
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
            goto Error;
        
        case WM_HSCROLL:
            result = OnHScroll (hDlg, lParam);
            goto Error;
        
        case WM_COMMAND:
            result = OnCommand (hDlg, wParam);
            goto Error;
        
        case WM_DESTROY:
        {
            DialogContext * pContext = GetDialogContext (hDlg);
            ConfigDialogController * pController = pContext ? pContext->m_controller.get() : nullptr;

            if (pController && pController->IsLiveMode())
            {
                if (Application * pApp = GetApplicationFromDialog (hDlg))
                {
                    pApp->SetConfigDialog (nullptr);
                }

                PostQuitMessage (0);
            }

            if (pContext)
            {
                SetWindowLongPtr (hDlg, DWLP_USER, 0);
                
                if (pContext->m_ownsContextMemory)
                {
                    delete pContext;
                }
            }

            result = TRUE;
            goto Error;
        }
    }

Error:
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
    


    dlgContext.m_controller = std::make_unique<ConfigDialogController>();
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



    context->m_controller = std::make_unique<ConfigDialogController>();
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
