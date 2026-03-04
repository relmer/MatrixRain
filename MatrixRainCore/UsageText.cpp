#include "pch.h"

#include "UsageText.h"

#include "UnicodeSymbols.h"
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
    // Populate switch table — general options first, then screensaver options
    //

    m_switches =
    {
        { L'c', L"",       L"Show settings dialog",                         false },
        { L'?', L"",       L"Display this help message",                    false },
        { L's', L"",       L"Run as full-screen screensaver",               true  },
        { L'p', L"<HWND>", L"Preview in the specified window",              true  },
        { L'a', L"",       L"Password change (unsupported, exits quietly)", true  },
    };

    BuildFormattedLines();
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageText::BuildFormattedLines
//
//  Formats the usage text into section-grouped display-ready lines.
//  Splits switches into "Options:" and "Screensaver Options:" sections.
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
    // Options section (non-screensaver switches)
    //

    m_formattedLines.push_back (L"Options:");

    for (const auto & sw : m_switches)
    {
        if (sw.isScreensaverOption)
        {
            continue;
        }

        std::wstring switchStr = std::format (L"  {}{}", m_switchPrefix, sw.switchChar);

        if (!sw.argument.empty())
        {
            switchStr += L" " + sw.argument;
        }

        while (switchStr.size() < 18)
        {
            switchStr += L' ';
        }

        switchStr += UnicodeSymbols::EmDash;
        switchStr += L" ";
        switchStr += sw.description;
        m_formattedLines.push_back (switchStr);
    }

    //
    // Screensaver Options section
    //

    m_formattedLines.push_back (L"");
    m_formattedLines.push_back (L"Screensaver Options:");

    for (const auto & sw : m_switches)
    {
        if (!sw.isScreensaverOption)
        {
            continue;
        }

        std::wstring switchStr = std::format (L"  {}{}", m_switchPrefix, sw.switchChar);

        if (!sw.argument.empty())
        {
            switchStr += L" " + sw.argument;
        }

        while (switchStr.size() < 18)
        {
            switchStr += L' ';
        }

        switchStr += UnicodeSymbols::EmDash;
        switchStr += L" ";
        switchStr += sw.description;
        m_formattedLines.push_back (switchStr);
    }

    //
    // Build the single formatted text string
    //

    m_formattedText.clear();

    for (size_t i = 0; i < m_formattedLines.size(); i++)
    {
        m_formattedText += m_formattedLines[i];

        if (i + 1 < m_formattedLines.size())
        {
            m_formattedText += L"\n";
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageText::GetPlainText
//
//  Returns formatted text with CRLF line endings for MessageBox display.
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
