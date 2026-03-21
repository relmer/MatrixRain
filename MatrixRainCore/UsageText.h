#pragma once





struct SwitchEntry
{
    wchar_t      switchChar;
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
    explicit UsageText (wchar_t switchPrefix);


    // Queries
    const std::wstring                & GetFormattedText ()   const { return m_formattedText;   }
    const std::vector<std::wstring>   & GetFormattedLines ()  const { return m_formattedLines;  }
    wchar_t                             GetSwitchPrefix ()    const { return m_switchPrefix;    }
    std::wstring                        GetPlainText ()       const;


    // Overlay-format entries (two-column pairs for Overlay consumption)
    std::vector<std::pair<std::wstring, std::wstring>> GetOverlayEntries () const;


private:
    void BuildFormattedLines ();


    wchar_t                    m_switchPrefix = L'/';
    std::vector<SwitchEntry>   m_switches;
    std::vector<std::wstring>  m_formattedLines;
    std::wstring               m_formattedText;
};

