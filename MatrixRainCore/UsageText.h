#pragma once





struct SwitchEntry
{
    wchar_t      switchChar;    // Single-char switch (e.g., L'c'), or L'\0' for multi-char
    std::wstring switchName;    // Multi-char switch name (e.g., "install"), empty for single-char
    std::wstring argument;
    std::wstring description;
};




struct HotkeyEntry
{
    std::wstring keyName;
    std::wstring description;
};





class UsageText
{
public:
    explicit UsageText (std::wstring_view switchPrefix);


    // Queries
    const std::wstring                & GetFormattedText ()   const { return m_formattedText;   }
    const std::vector<std::wstring>   & GetFormattedLines ()  const { return m_formattedLines;  }
    const std::wstring                & GetSwitchPrefix ()    const { return m_switchPrefix;    }
    std::wstring                        GetPlainText ()       const;


    // Overlay-format entries (two-column pairs for Overlay consumption)
    std::vector<std::pair<std::wstring, std::wstring>> GetOverlayEntries () const;


private:
    void BuildFormattedLines ();


    std::wstring               m_switchPrefix = L"/";
    std::vector<SwitchEntry>   m_switches;
    std::vector<std::wstring>  m_formattedLines;
    std::wstring               m_formattedText;
};

