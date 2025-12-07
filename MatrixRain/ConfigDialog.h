#pragma once

#include <Windows.h>




class Application;
class ApplicationState;
class ConfigDialogController;
struct ScreenSaverModeContext;




/// <summary>
/// Show modal configuration dialog (Control Panel mode).
/// </summary>
/// <param name="hInstance">Application instance handle</param>
/// <param name="context">Screensaver mode context with optional parent HWND</param>
/// <returns>Dialog result code, or -1 on failure</returns>
int ShowConfigDialog (HINSTANCE hInstance, const ScreenSaverModeContext & context);




/// <summary>
/// Create modeless configuration dialog over running application (live overlay mode).
/// </summary>
/// <param name="hInstance">Application instance handle</param>
/// <param name="parentHwnd">Parent window handle (application main window)</param>
/// <param name="pApplication">Pointer to running Application instance</param>
/// <param name="pAppState">Pointer to ApplicationState for live updates</param>
/// <param name="phDlg">Output parameter receiving dialog HWND</param>
/// <returns>S_OK on success, error HRESULT otherwise</returns>
HRESULT CreateConfigDialog (HINSTANCE          hInstance,
                             HWND               parentHwnd,
                             Application      * pApplication,
                             ApplicationState * pAppState,
                             HWND             * phDlg);
