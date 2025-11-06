// MatrixRain - Entry Point
// This file will be implemented in later phases with Application class integration

#include <Windows.h>

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // TODO: Initialize Application class and run main loop
    // This will be implemented in Phase 3 (User Story 1)
    
    MessageBoxW(nullptr, L"MatrixRain - Coming Soon!", L"Matrix Rain", MB_OK);
    
    return 0;
}
