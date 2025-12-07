#include "pch.h"

#include "..\MatrixRainCore\Application.h"
#include "..\MatrixRainCore\ScreenSaverModeParser.h"
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
    hr = ParseCommandLine (lpCmdLine, context);
    CHR (hr);

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
