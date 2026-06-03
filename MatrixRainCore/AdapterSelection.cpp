#include "pch.h"

#include "AdapterSelection.h"




////////////////////////////////////////////////////////////////////////////////
//
//  ResolveAdapter
//
////////////////////////////////////////////////////////////////////////////////

std::optional<LUID> ResolveAdapter (const std::vector<AdapterInfo> & adapters,
                                    const std::wstring             & savedDescription)
{
    if (savedDescription.empty())
    {
        return std::nullopt;
    }

    for (const AdapterInfo & adapter : adapters)
    {
        if (adapter.m_description == savedDescription)
        {
            return adapter.m_luid;
        }
    }

    return std::nullopt;
}




////////////////////////////////////////////////////////////////////////////////
//
//  FormatAdapterLabel
//
////////////////////////////////////////////////////////////////////////////////

std::wstring FormatAdapterLabel (const AdapterInfo & adapter)
{
    if (adapter.m_isDefault)
    {
        return adapter.m_description + L" (default)";
    }

    return adapter.m_description;
}
