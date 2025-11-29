#include "pch.h"
#include "MatrixRain/Application.h"





int APIENTRY wWinMain(_In_     HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_     LPWSTR    lpCmdLine,
                      _In_     int       nCmdShow)
{
    UNREFERENCED_PARAMETER (hPrevInstance);
    UNREFERENCED_PARAMETER (lpCmdLine);
    
    MatrixRain::Application app;
    HRESULT                 hr = S_OK;
    int                     retval = 0;



    // Set DPI awareness to per-monitor V2 for consistent physical pixel measurements
    SetProcessDpiAwarenessContext (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    hr = app.Initialize (hInstance, nCmdShow);
    CHR (hr);
    
    retval = app.Run();


Error:
    if (FAILED (hr))
    {
        MessageBoxW (nullptr, L"Failed to initialize MatrixRain", L"Initialization Error", MB_OK | MB_ICONERROR);
        retval = -1;
    }

    return retval;
}
