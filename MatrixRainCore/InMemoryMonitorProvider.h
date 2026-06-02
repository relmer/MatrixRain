#pragma once

#include "IMonitorProvider.h"




////////////////////////////////////////////////////////////////////////////////
//
//  InMemoryMonitorProvider
//
//  Test IMonitorProvider that returns a fixed list of monitor descriptors
//  supplied at construction.  Lets orchestration tests exercise N-monitor
//  layouts deterministically without any real displays or D3D.
//
////////////////////////////////////////////////////////////////////////////////

class InMemoryMonitorProvider : public IMonitorProvider
{
public:
    explicit InMemoryMonitorProvider (std::vector<MonitorInfo> monitors) :
        m_monitors (std::move (monitors))
    {
    }

    std::vector<MonitorInfo> GetMonitors() const override
    {
        return m_monitors;
    }

private:
    std::vector<MonitorInfo> m_monitors;
};
