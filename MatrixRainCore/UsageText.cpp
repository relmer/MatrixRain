#include "pch.h"

#include "UsageText.h"

#include "Version.h"





////////////////////////////////////////////////////////////////////////////////
//
//  UsageText::UsageText
//
//  Populates switch and hotkey tables, builds formatted lines.
//
////////////////////////////////////////////////////////////////////////////////

UsageText::UsageText (wchar_t switchPrefix) :
    m_switchPrefix (switchPrefix)
{
    //
    // Populate switch table
    //

    m_switches =
    {
        { L's', L"",       L"Run as full-screen screensaver"                },
        { L'p', L"<HWND>", L"Preview in the specified window"              },
        { L'c', L"",       L"Show settings dialog"                         },
        { L'a', L"",       L"Password change (unsupported, exits quietly)" },
        { L'?', L"",       L"Display this help message"                    },
    };

    //
    // Populate hotkey table
    //

    m_hotkeys =
    {
        { L"Space",     L"Pause / resume animation"       },
        { L"Enter",     L"Open settings dialog"           },
        { L"?",         L"Show this help information"      },
        { L"C",         L"Cycle color scheme"              },
        { L"S",         L"Toggle screensaver mode"         },
        { L"+/-",       L"Adjust rain density"             },
        { L"`",         L"Toggle FPS counter"              },
        { L"Alt+Enter", L"Toggle fullscreen"               },
        { L"Esc",       L"Exit"                            },
    };

    BuildFormattedLines();
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageText::BuildFormattedLines
//
//  Formats the usage text into display-ready lines.
//
////////////////////////////////////////////////////////////////////////////////

void UsageText::BuildFormattedLines ()
{
    m_formattedLines.clear();

    //
    // Header
    //

    m_formattedLines.push_back (std::format (L"MatrixRain v{}.{}.{}", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD));
    m_formattedLines.push_back (L"");

    //
    // Usage line
    //

    m_formattedLines.push_back (std::format (L"Usage: MatrixRain.exe [{}option]", m_switchPrefix));
    m_formattedLines.push_back (L"");

    //
    // Switches
    //

    m_formattedLines.push_back (L"Options:");

    for (const auto & sw : m_switches)
    {
        std::wstring switchStr = std::format (L"  {}{}", m_switchPrefix, sw.switchChar);

        if (!sw.argument.empty())
        {
            switchStr += L" " + sw.argument;
        }

        // Pad to column width for description alignment
        while (switchStr.size() < 18)
        {
            switchStr += L' ';
        }

        switchStr += sw.description;
        m_formattedLines.push_back (switchStr);
    }

    //
    // Hotkeys
    //

    m_formattedLines.push_back (L"");
    m_formattedLines.push_back (L"Hotkeys:");

    for (const auto & hk : m_hotkeys)
    {
        std::wstring keyStr = std::format (L"  {}", hk.keyName);

        while (keyStr.size() < 18)
        {
            keyStr += L' ';
        }

        keyStr += hk.description;
        m_formattedLines.push_back (keyStr);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageText::GetPlainText
//
//  Returns all formatted lines joined with CRLF.
//
////////////////////////////////////////////////////////////////////////////////

std::wstring UsageText::GetPlainText () const
{
    std::wstring result;



    for (size_t i = 0; i < m_formattedLines.size(); i++)
    {
        result += m_formattedLines[i];

        if (i + 1 < m_formattedLines.size())
        {
            result += L"\r\n";
        }
    }

    return result;
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageText::DetectSwitchPrefix
//
//  Scans command line for /? or -? and returns the prefix character.
//
////////////////////////////////////////////////////////////////////////////////

wchar_t UsageText::DetectSwitchPrefix (std::wstring_view commandLine)
{
    if (commandLine.find (L"-?") != std::wstring_view::npos)
    {
        return L'-';
    }

    return L'/';
}
