#include "pch.h"

#include "..\MatrixRainCore\Application.h"
#include "..\MatrixRainCore\ScreenSaverInstaller.h"
#include "..\MatrixRainCore\WindowsRegistryProvider.h"
#include "..\MatrixRainCore\WindowsFileSystemProvider.h"
#include "..\MatrixRainCore\CommandLine.h"
#include "..\MatrixRainCore\UsageText.h"
#include "ConfigDialog.h"





////////////////////////////////////////////////////////////////////////////////
//
//  wWinMain
//
//  Application entry point.
//
////////////////////////////////////////////////////////////////////////////////

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
    {
        CommandLine cmdLine;

        hr = cmdLine.Parse (lpCmdLine, context);
        CHR (hr);
    }

    // Handle install/uninstall — early exit before application initialization
    if (context.m_mode == ScreenSaverMode::Install || context.m_mode == ScreenSaverMode::Uninstall)
    {
        LPCWSTR pszSwitch = (context.m_mode == ScreenSaverMode::Uninstall)  ? L"/uninstall" :
                            context.m_forceInstall                          ? L"/install /force" :
                                                                              L"/install";

        // Check for policies that would block the screensaver (before UAC prompt)
        // /force or --force skips the policy check
        if (context.m_mode == ScreenSaverMode::Install && !context.m_forceInstall)
        {
            std::wstring policyWarning;
            bool         fBlocked = false;


            hr = ScreenSaverInstaller::CheckScreenSaverPolicies (fBlocked, policyWarning);
            CHR (hr);

            if (fBlocked)
            {
                MessageBoxW (nullptr, policyWarning.c_str(), L"MatrixRain", MB_OK | MB_ICONERROR);
                goto Error;
            }
        }

        if (!ScreenSaverInstaller::IsElevated())
        {
            hr = ScreenSaverInstaller::RequestElevation (pszSwitch);
            goto Error;
        }

        if (context.m_mode == ScreenSaverMode::Install)
        {
            hr = ScreenSaverInstaller::Install();
        }
        else
        {
            WindowsRegistryProvider     registry;
            WindowsFileSystemProvider   fileSystem;

            hr = ScreenSaverInstaller::Uninstall (registry, fileSystem);

            if (hr == S_OK)
            {
                MessageBoxW (nullptr, L"MatrixRain screensaver has been uninstalled.", L"MatrixRain", MB_OK | MB_ICONINFORMATION);
            }
            else if (hr == S_FALSE)
            {
                MessageBoxW (nullptr, L"MatrixRain screensaver is not installed.", L"MatrixRain", MB_OK | MB_ICONINFORMATION);
            }
        }

        goto Error;
    }

    // Handle help request — runs through Application in HelpRequested mode
    // (no longer uses UsageDialog — overlay renders via GPU pipeline)

    // Handle configuration mode with HWND (modal Control Panel dialog)
    if (context.m_mode == ScreenSaverMode::SettingsDialog && context.m_previewParentHwnd != nullptr)
    {
        retval = ShowConfigDialog (hInstance, context);
        goto Error;
    }

    // For live overlay mode (/c without HWND) or normal operation, initialize the application
    hr = app.Initialize (hInstance, nCmdShow, &context);
    CHR (hr);
    
    // If this was a live overlay config request, create modeless dialog
    if (context.m_mode == ScreenSaverMode::SettingsDialog)
    {
        HWND hConfigDialog = nullptr;

        hr = CreateConfigDialog (hInstance,
                                app.GetMainWindowHwnd(),
                                &app,
                                app.GetApplicationState(),
                                &hConfigDialog);
        CHRA (hr);
        
        app.SetConfigDialog (hConfigDialog);
    }

    // Wire Enter key → open config dialog callback
    app.SetOpenConfigDialogCallback ([&app, hInstance]()
    {
        HWND    hConfigDialog = nullptr;
        HRESULT hrDialog      = S_OK;

        hrDialog = CreateConfigDialog (hInstance,
                                       app.GetMainWindowHwnd(),
                                       &app,
                                       app.GetApplicationState(),
                                       &hConfigDialog);

        if (SUCCEEDED (hrDialog))
        {
            app.SetConfigDialog (hConfigDialog);
        }
    });

    // Wire ? key → show usage dialog callback
    app.SetShowUsageDialogCallback ([&app]()
    {
        static bool s_fDialogOpen = false;

        if (s_fDialogOpen)
        {
            return;
        }

        s_fDialogOpen = true;

        UsageText usage (L"/");
        MessageBoxW (app.GetMainWindowHwnd(), usage.GetPlainText().c_str(), L"MatrixRain — Help", MB_OK | MB_ICONINFORMATION);

        s_fDialogOpen = false;
    });
    
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
