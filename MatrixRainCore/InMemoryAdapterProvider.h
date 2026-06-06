#pragma once

#include "IAdapterProvider.h"




////////////////////////////////////////////////////////////////////////////////
//
//  InMemoryAdapterProvider
//
//  Test-only IAdapterProvider that returns a copy of the vector it was
//  constructed with.  No DXGI dependency.
//
////////////////////////////////////////////////////////////////////////////////

class InMemoryAdapterProvider : public IAdapterProvider
{
public:
    explicit InMemoryAdapterProvider (std::vector<AdapterInfo> adapters) :
        m_adapters (std::move (adapters))
    {
    }

    std::vector<AdapterInfo> EnumerateAdapters() const override
    {
        return m_adapters;
    }

private:
    std::vector<AdapterInfo> m_adapters;
};
