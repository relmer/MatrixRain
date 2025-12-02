#include "pch.h"

#include "matrixrain/ScreenSaverModeParser.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SkipWhitespace
//
//  Advances the pointer past any whitespace characters
//
////////////////////////////////////////////////////////////////////////////////

static inline void SkipWhitespace (LPCWSTR & psz)
{
    while (*psz == L' ' || *psz == L'\t')
    {
        psz++;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  SetContextFlagsFromMode
//
//  Sets additional context flags based on the selected ScreenSaverMode
//
////////////////////////////////////////////////////////////////////////////////

static void SetContextFlagsFromMode (ScreenSaverModeContext & context, HWND hwnd)
{
    switch (context.m_mode)
    {
        case ScreenSaverMode::ScreenSaverFull:
            context.m_enableHotkeys = false;
            context.m_hideCursor    = true;
            context.m_exitOnInput   = true;
            context.m_suppressDebug = true;
            break;

        case ScreenSaverMode::ScreenSaverPreview:
            ASSERT (hwnd != 0);
            context.m_previewParentHwnd = hwnd;
            context.m_enableHotkeys     = false;
            context.m_hideCursor        = false;
            context.m_exitOnInput       = false;
            context.m_suppressDebug     = true;
            break;

        case ScreenSaverMode::SettingsDialog:
            context.m_enableHotkeys = false;
            context.m_hideCursor    = false;
            context.m_exitOnInput   = false;
            if (hwnd != nullptr)
            {
                context.m_previewParentHwnd = hwnd;
            }
            break;

        case ScreenSaverMode::PasswordChangeUnsupported:
            context.m_enableHotkeys = false;
            context.m_hideCursor    = false;
            context.m_exitOnInput   = false;
            break;

        case ScreenSaverMode::Normal:
        default:
            // Fallback to Normal mode on unexpected value
            context.m_mode          = ScreenSaverMode::Normal;
            context.m_enableHotkeys = true;
            context.m_hideCursor    = false;
            context.m_exitOnInput   = false;
            context.m_suppressDebug = false;
            break;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ParseCommandLine
//
//  Parses screensaver command-line arguments and constructs ScreenSaverModeContext
//
////////////////////////////////////////////////////////////////////////////////

ScreenSaverModeContext ParseCommandLine (LPCWSTR pszCommandLine)
{
    HRESULT                hr        = S_OK;
    ScreenSaverModeContext context;
    bool                   hasSwitch = false;   
    wchar_t                cmd       = L'\0';    // To store the command character
    HWND                   hwnd      = nullptr;  // To store parsed HWND value for /p and /c commands



    ASSERT (context.m_mode == ScreenSaverMode::Normal);

    // Default to normal if there's no command line
    BAIL_OUT_IF (!pszCommandLine, S_OK);

    SkipWhitespace (pszCommandLine);

    // Check for screensaver arguments (starts with / or -)
    hasSwitch = (*pszCommandLine == L'/' || *pszCommandLine == L'-');
    BAIL_OUT_IF (!hasSwitch, S_OK);

    // Skip the / or - prefix
    pszCommandLine++;

    // Get the command character (case-insensitive)
    cmd = towlower (*pszCommandLine);
    switch (cmd)
    {
        case L's':  //  /s - Full-screen screensaver
            context.m_mode = ScreenSaverMode::ScreenSaverFull;
            break;

        case L'p':  //  /p <HWND> - Preview mode
            pszCommandLine++;  // Skip 'p'
            SkipWhitespace (pszCommandLine);

            // Try to parse HWND as decimal integer.  If this fails, we'll stay in Normal mode.
            hwnd = (HWND) wcstoull (pszCommandLine, nullptr, 10);
            if (hwnd != 0)
            {
                context.m_mode = ScreenSaverMode::ScreenSaverPreview;
            }
            break;

        case L'c':  // /c or /c:<HWND> - Configuration dialog
        {
            context.m_mode = ScreenSaverMode::SettingsDialog;
            
            // Check for optional :<HWND>
            pszCommandLine++;  // Skip 'c'
            if (*pszCommandLine == L':')
            {
                pszCommandLine++;  // Skip ':'
                hwnd = (HWND) wcstoull (pszCommandLine, NULL, 10);
            }
            break;
        }

        case L'a':  // /a - Password change (unsupported)
            context.m_mode = ScreenSaverMode::PasswordChangeUnsupported;
            break;

        default:  // Unknown argument, default to Normal
            context.m_mode = ScreenSaverMode::Normal;
            break;
    }



Error:
    SetContextFlagsFromMode (context, hwnd);

    return context;
}
