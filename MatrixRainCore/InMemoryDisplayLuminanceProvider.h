#pragma once

#include "IDisplayLuminanceProvider.h"





////////////////////////////////////////////////////////////////////////////////
//
//  InMemoryDisplayLuminanceProvider
//
//  Test-only IDisplayLuminanceProvider. Answers Query from a queue of
//  scripted snapshots, repeating the last one once the queue is down to it,
//  so a test can play an SDR -> HDR -> SDR sequence or a white-level change
//  and watch what the tracker does. The stale flag is set by the test and
//  clears itself after one IsStale, the way a refreshed factory would. No
//  DXGI dependency; the swap chain argument is ignored.
//
////////////////////////////////////////////////////////////////////////////////

class InMemoryDisplayLuminanceProvider : public IDisplayLuminanceProvider
{
public:
    explicit InMemoryDisplayLuminanceProvider (std::vector<DisplayLuminance> script) :
        m_script (std::move (script))
    {
    }

    DisplayLuminance Query (IDXGISwapChain1 * /* pSwapChain */) noexcept override
    {
        ++m_queryCount;

        if (m_script.empty())
        {
            return DisplayLuminance {};
        }

        DisplayLuminance next = m_script.front();


        if (m_script.size() > 1)
        {
            m_script.erase (m_script.begin());
        }

        return next;
    }

    bool IsStale() noexcept override
    {
        const bool stale = m_stale;


        m_stale = false;

        return stale;
    }

    // Test controls
    void Push     (const DisplayLuminance & luminance) { m_script.push_back (luminance); }
    void SetStale (bool stale) noexcept                { m_stale = stale; }

    int  QueryCount() const noexcept                   { return m_queryCount; }

private:
    std::vector<DisplayLuminance> m_script;
    bool                          m_stale      { false };
    int                           m_queryCount { 0 };
};
