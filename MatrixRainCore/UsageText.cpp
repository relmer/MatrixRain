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
    // Populate switch table — general options only (screensaver options not
    // shown; they're internal Windows screensaver protocol, not for users)
    //

    m_switches =
    {
        { L'c', L"", L"Show settings dialog"      },
        { L'?', L"", L"Display this help message" },
    };

    BuildFormattedLines();
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageText::BuildFormattedLines
//
//  Formats the usage text into display-ready lines with version, copyright,
//  and command-line switch descriptions.
//
////////////////////////////////////////////////////////////////////////////////

void UsageText::BuildFormattedLines ()
{
    m_formattedLines.clear();

    //
    // Header — version, architecture, build timestamp, copyright
    //

    std::wstring buildTimestamp = VERSION_BUILD_TIMESTAMP;

    m_formattedLines.push_back (std::format (L"MatrixRain v{} {} ({})",
                                             VERSION_WSTRING,
                                             VERSION_ARCH_WSTRING,
                                             buildTimestamp));
    m_formattedLines.push_back (std::format (L"Copyright {} 2024-{} by Robert Elmer",
                                             UnicodeSymbols::Copyright,
                                             VERSION_YEAR_WSTRING));
    m_formattedLines.push_back (L"");

    //
    // Usage line
    //

    m_formattedLines.push_back (std::format (L"Usage: MatrixRain.exe [{}option]", m_switchPrefix));
    m_formattedLines.push_back (L"");

    //
    // Options section
    //

    m_formattedLines.push_back (L"Options:");

    for (const auto & sw : m_switches)
    {
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
//  UsageText::GetOverlayEntries
//
//  Returns two-column pairs suitable for Overlay consumption.
//  Single-column rows have an empty second element.
//
////////////////////////////////////////////////////////////////////////////////

std::vector<std::pair<std::wstring, std::wstring>> UsageText::GetOverlayEntries () const
{
    std::vector<std::pair<std::wstring, std::wstring>> entries;



    entries.push_back ({ std::format (L"MatrixRain v{} {} ({})",
                                      VERSION_WSTRING,
                                      VERSION_ARCH_WSTRING,
                                      std::wstring (VERSION_BUILD_TIMESTAMP)), L"" });
    entries.push_back ({ std::format (L"Copyright {} 2024-{} by Robert Elmer",
                                      UnicodeSymbols::Copyright,
                                      VERSION_YEAR_WSTRING), L"" });
    entries.push_back ({ L"", L"" });
    entries.push_back ({ std::format (L"Usage: MatrixRain.exe [{}option]", m_switchPrefix), L"" });
    entries.push_back ({ L"", L"" });
    entries.push_back ({ L"Options:", L"" });

    for (const auto & sw : m_switches)
    {
        entries.push_back ({ std::format (L"  {}{}", m_switchPrefix, sw.switchChar), sw.description });
    }

    return entries;
}
