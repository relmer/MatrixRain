#include "pch.h"

#include "CommandLine.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CommandLine::SkipWhitespace
//
//  Advances the pointer past any whitespace characters.
//
////////////////////////////////////////////////////////////////////////////////

void CommandLine::SkipWhitespace (LPCWSTR & psz)
{
    while (*psz == L' ' || *psz == L'\t')
    {
        psz++;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CommandLine::SetContextFlagsFromMode
//
//  Sets additional context flags based on the selected ScreenSaverMode.
//
////////////////////////////////////////////////////////////////////////////////

void CommandLine::SetContextFlagsFromMode (ScreenSaverModeContext & context)
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
            ASSERT (context.m_previewParentHwnd != 0);
            context.m_enableHotkeys = false;
            context.m_hideCursor    = false;
            context.m_exitOnInput   = false;
            context.m_suppressDebug = true;
            break;

        case ScreenSaverMode::SettingsDialog:
            context.m_enableHotkeys = false;
            context.m_hideCursor    = false;
            context.m_exitOnInput   = false;
            break;

        case ScreenSaverMode::PasswordChangeUnsupported:
        case ScreenSaverMode::HelpRequested:
        case ScreenSaverMode::Install:
        case ScreenSaverMode::Uninstall:
            context.m_enableHotkeys = false;
            context.m_hideCursor    = false;
            context.m_exitOnInput   = false;
            break;

        case ScreenSaverMode::Normal:
        default:
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
//  CommandLine::TryParseMultiCharSwitch
//
//  Attempts to match a multi-character switch word (e.g., "install", "uninstall")
//  starting at pszArg.  If matched, sets context.m_mode and returns S_OK.
//  If the word is unrecognized, sets context.m_errorMessage and returns
//  E_INVALIDARG.  If there's only a single character, returns S_FALSE (caller
//  should fall through to single-character parsing).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CommandLine::TryParseMultiCharSwitch (LPCWSTR                  pszArg,
                                              ScreenSaverModeContext & context)
{
    HRESULT      hr         = S_OK;
    LPCWSTR      pszStart   = pszArg;
    size_t       cchSwitch  = 0;
    std::wstring switchWord;



    while (iswalpha (*pszArg))
    {
        pszArg++;
    }

    cchSwitch = pszArg - pszStart;
    BAIL_OUT_IF (cchSwitch <= 1, S_FALSE);

    switchWord.assign (pszStart, cchSwitch);
    for (auto & ch : switchWord)
    {
        ch = towlower (ch);
    }

    if (switchWord == L"install")
    {
        context.m_mode = ScreenSaverMode::Install;
    }
    else if (switchWord == L"uninstall")
    {
        context.m_mode = ScreenSaverMode::Uninstall;
    }
    else if (switchWord == L"force")
    {
        // --force is only valid as a modifier for --install
        context.m_errorMessage = std::format (L"{}force is valid only with {}install",
                                              context.m_switchPrefix,
                                              context.m_switchPrefix);
        CHR (E_INVALIDARG);
    }
    else
    {
        context.m_errorMessage = std::format (L"Unrecognized command-line switch: {}{}",
                                              context.m_switchPrefix,
                                              switchWord);
        CHR (E_INVALIDARG);
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CommandLine::ValidateRemainingArgs
//
//  After parsing the primary switch, checks whether any trailing arguments
//  remain.  Only /install (or --install) accepts a trailing modifier (/force).
//  All other switches reject additional arguments.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CommandLine::ValidateRemainingArgs (LPCWSTR                & pszCommandLine,
                                            ScreenSaverModeContext & context)
{
    HRESULT      hr       = S_OK;
    LPCWSTR      pszStart = nullptr;
    size_t       cchWord  = 0;
    std::wstring word;



    SkipWhitespace (pszCommandLine);
    BAIL_OUT_IF (*pszCommandLine == L'\0', S_OK);

    // Only --install / /install accepts additional arguments (/force)
    if (context.m_mode != ScreenSaverMode::Install)
    {
        context.m_errorMessage = L"This option does not accept additional arguments";
        CHR (E_INVALIDARG);
    }

    // Must start with a switch prefix (/ or --)
    if (*pszCommandLine == L'/')
    {
        pszCommandLine++;
    }
    else if (*pszCommandLine == L'-' && *(pszCommandLine + 1) == L'-')
    {
        pszCommandLine += 2;
    }
    else
    {
        // Extract the full invalid token for the error message
        LPCWSTR pszTokenStart = pszCommandLine;
        while (*pszCommandLine != L'\0' && *pszCommandLine != L' ' && *pszCommandLine != L'\t')
        {
            pszCommandLine++;
        }

        std::wstring invalidToken (pszTokenStart, pszCommandLine - pszTokenStart);
        context.m_errorMessage = std::format (L"Invalid argument: {}", invalidToken);
        CHR (E_INVALIDARG);
    }

    // Extract the word
    pszStart = pszCommandLine;

    while (iswalpha (*pszCommandLine))
    {
        pszCommandLine++;
    }

    cchWord = pszCommandLine - pszStart;
    word.assign (pszStart, cchWord);

    for (auto & ch : word)
    {
        ch = towlower (ch);
    }

    if (word != L"force")
    {
        context.m_errorMessage = std::format (L"{}install only accepts {}force as an additional option",
                                              context.m_switchPrefix,
                                              context.m_switchPrefix);
        CHR (E_INVALIDARG);
    }

    context.m_forceInstall = true;

    // Nothing should follow force
    SkipWhitespace (pszCommandLine);

    if (*pszCommandLine != L'\0')
    {
        context.m_errorMessage = std::format (L"Unexpected arguments after {}force",
                                              context.m_switchPrefix);
        CHR (E_INVALIDARG);
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CommandLine::HandleScreenSaver
//
//  /s — Launch full-screen screensaver mode.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CommandLine::HandleScreenSaver (LPCWSTR & /*pszCommandLine*/, ScreenSaverModeContext & context)
{
    context.m_mode = ScreenSaverMode::ScreenSaverFull;
    return S_OK;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CommandLine::HandleScreenSaverPreview
//
//  /p <HWND> — Preview mode parented to the specified window handle.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CommandLine::HandleScreenSaverPreview (LPCWSTR & pszCommandLine, ScreenSaverModeContext & context)
{
    pszCommandLine++;
    SkipWhitespace (pszCommandLine);

    HWND hwnd = (HWND) wcstoull (pszCommandLine, nullptr, 10);
    if (hwnd != 0)
    {
        context.m_mode              = ScreenSaverMode::ScreenSaverPreview;
        context.m_previewParentHwnd = hwnd;
    }

    return S_OK;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CommandLine::HandleConfigDialog
//
//  /c or /c:<HWND> — Show configuration dialog, optionally parented to HWND.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CommandLine::HandleConfigDialog (LPCWSTR & pszCommandLine, ScreenSaverModeContext & context)
{
    context.m_mode = ScreenSaverMode::SettingsDialog;

    pszCommandLine++;
    if (*pszCommandLine == L':')
    {
        pszCommandLine++;
        HWND hwnd = (HWND) wcstoull (pszCommandLine, nullptr, 10);
        if (hwnd != 0)
        {
            context.m_previewParentHwnd = hwnd;
        }
    }

    return S_OK;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CommandLine::HandlePasswordChange
//
//  /a — Password change request (unsupported, exits immediately).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CommandLine::HandlePasswordChange (LPCWSTR & /*pszCommandLine*/, ScreenSaverModeContext & context)
{
    context.m_mode = ScreenSaverMode::PasswordChangeUnsupported;
    return S_OK;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CommandLine::HandleHelp
//
//  /? or -? — Display usage/help text.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CommandLine::HandleHelp (LPCWSTR & /*pszCommandLine*/, ScreenSaverModeContext & context)
{
    context.m_mode = ScreenSaverMode::HelpRequested;
    return S_OK;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CommandLine::Parse
//
//  Parses screensaver command-line arguments and constructs ScreenSaverModeContext.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CommandLine::Parse (LPCWSTR pszCommandLine, ScreenSaverModeContext & context)
{
    HRESULT hr        = S_OK;
    bool    hasSwitch = false;
    wchar_t cmd       = L'\0';



    ASSERT (context.m_mode == ScreenSaverMode::Normal);

    // Default to normal if there's no command line
    BAIL_OUT_IF (!pszCommandLine, S_OK);

    SkipWhitespace (pszCommandLine);

    // Check for screensaver arguments (starts with / or -)
    hasSwitch = (*pszCommandLine == L'/' || *pszCommandLine == L'-');
    BAIL_OUT_IF (!hasSwitch, S_OK);

    // Remember which prefix the user used
    context.m_switchPrefix = std::wstring (1, *pszCommandLine);

    // Skip the / or - prefix
    pszCommandLine++;

    // Check for -- prefix (long option style, only valid with -)
    if (context.m_switchPrefix == L"-" && *pszCommandLine == L'-')
    {
        context.m_switchPrefix = L"--";
        pszCommandLine++;

        hr = TryParseMultiCharSwitch (pszCommandLine, context);
        CHR (hr);

        // S_FALSE means single char after -- which is invalid
        if (hr == S_FALSE)
        {
            context.m_errorMessage = std::format (L"Unrecognized command-line switch: --{}", *pszCommandLine);
            CHR (E_INVALIDARG);
        }

        // Advance past the matched word
        while (iswalpha (*pszCommandLine)) pszCommandLine++;

        hr = ValidateRemainingArgs (pszCommandLine, context);
        CHR (hr);

        BAIL_OUT_IF (true, hr);
    }

    // With / prefix, try multi-char switch matching (e.g., /install, /uninstall)
    if (context.m_switchPrefix == L"/")
    {
        hr = TryParseMultiCharSwitch (pszCommandLine, context);
        CHR (hr);

        if (hr == S_OK)
        {
            // Advance past the matched word
            while (iswalpha (*pszCommandLine)) pszCommandLine++;

            hr = ValidateRemainingArgs (pszCommandLine, context);
            CHR (hr);

            BAIL_OUT_IF (true, hr);
        }

        // S_FALSE means single char — fall through to table lookup below
        hr = S_OK;
    }

    // Table-driven single-character switch dispatch
    cmd = towlower (*pszCommandLine);

    for (const auto & entry : s_krgSingleCharSwitches)
    {
        if (entry.ch == cmd)
        {
            hr = (this->*entry.pfnHandler) (pszCommandLine, context);
            CHR (hr);

            // Advance past the switch char if the handler didn't consume it
            if (towlower (*pszCommandLine) == cmd)
            {
                pszCommandLine++;
            }

            // Skip past any remaining argument consumed by the handler
            while (*pszCommandLine != L'\0' && *pszCommandLine != L' ' &&
                   *pszCommandLine != L'\t' && *pszCommandLine != L'/' &&
                   *pszCommandLine != L'-')
            {
                pszCommandLine++;
            }

            hr = ValidateRemainingArgs (pszCommandLine, context);
            CHR (hr);

            BAIL_OUT_IF (true, hr);
        }
    }

    // No match — unrecognized switch
    context.m_errorMessage = std::format (L"Unrecognized command-line switch: {}{}", context.m_switchPrefix, cmd);
    CHR (E_INVALIDARG);

Error:
    SetContextFlagsFromMode (context);

    return hr;
}
