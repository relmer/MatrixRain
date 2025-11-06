#include "pch.h"
#include "matrixrain/Application.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MatrixRain::Application app;
    
    if (!app.Initialize(hInstance, nCmdShow))
    {
        MessageBoxW(nullptr, L"Failed to initialize Matrix Rain application.\n\nCheck that DirectX 11 is available.", 
                    L"Initialization Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    return app.Run();
}
