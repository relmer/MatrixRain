#include "pch.h"
#include "MatrixRain/Application.h"

int APIENTRY wWinMain(_In_     HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_     LPWSTR    lpCmdLine,
                      _In_     int       nCmdShow)
{
    UNREFERENCED_PARAMETER (hPrevInstance);
    UNREFERENCED_PARAMETER (lpCmdLine);

    // Set DPI awareness to per-monitor V2 for consistent physical pixel measurements
    SetProcessDpiAwarenessContext (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    MatrixRain::Application app;
    
    HRESULT hr = app.Initialize (hInstance, nCmdShow);
    if (FAILED (hr))
    {
        MessageBoxW (nullptr, 
                     L"Failed to initialize MatrixRain", 
                     L"Initialization Error", 
                     MB_OK | MB_ICONERROR);
        return -1;
    }

    return app.Run();
}
