#pragma once

#include "ScreenSaverModeContext.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CommandLine
//
//  Parses command-line arguments into a ScreenSaverModeContext.
//  Handles single-character switches (/s, /c, /p, /a, /?), multi-character
//  switches (/install, --uninstall), and prefix detection (/, -, --).
//
////////////////////////////////////////////////////////////////////////////////

class CommandLine
{
public:
    HRESULT Parse (LPCWSTR pszCommandLine, ScreenSaverModeContext & context);

private:
    using SwitchHandlerFn = HRESULT (CommandLine::*)(LPCWSTR &, ScreenSaverModeContext &);

    struct SingleCharSwitch
    {
        wchar_t         ch;
        SwitchHandlerFn pfnHandler;
    };

    HRESULT TryParseMultiCharSwitch (LPCWSTR pszArg, ScreenSaverModeContext & context);

    HRESULT HandleScreenSaver        (LPCWSTR & pszCommandLine, ScreenSaverModeContext & context);
    HRESULT HandleScreenSaverPreview (LPCWSTR & pszCommandLine, ScreenSaverModeContext & context);
    HRESULT HandleConfigDialog       (LPCWSTR & pszCommandLine, ScreenSaverModeContext & context);
    HRESULT HandlePasswordChange     (LPCWSTR & pszCommandLine, ScreenSaverModeContext & context);
    HRESULT HandleHelp               (LPCWSTR & pszCommandLine, ScreenSaverModeContext & context);

    static constexpr SingleCharSwitch s_krgSingleCharSwitches[] =
    {
        { L's', &CommandLine::HandleScreenSaver        },
        { L'p', &CommandLine::HandleScreenSaverPreview },
        { L'c', &CommandLine::HandleConfigDialog       },
        { L'a', &CommandLine::HandlePasswordChange     },
        { L'?', &CommandLine::HandleHelp               },
    };

    static void SkipWhitespace          (LPCWSTR & psz);
    static void SetContextFlagsFromMode (ScreenSaverModeContext & context);
};
