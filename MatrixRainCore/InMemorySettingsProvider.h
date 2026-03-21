#pragma once

#include "ISettingsProvider.h"





////////////////////////////////////////////////////////////////////////////////
//
//  InMemorySettingsProvider
//
//  In-memory implementation of ISettingsProvider for unit tests.
//  Stores settings in memory instead of the Windows registry.
//
////////////////////////////////////////////////////////////////////////////////

class InMemorySettingsProvider : public ISettingsProvider
{
public:
    HRESULT Load (ScreenSaverSettings & settings) override
    {
        if (!m_hasData)
        {
            return S_FALSE;
        }

        settings = m_settings;
        settings.Clamp();
        return S_OK;
    }

    HRESULT Save (const ScreenSaverSettings & settings) override
    {
        m_settings = settings;
        m_hasData  = true;
        return S_OK;
    }

    void Clear()
    {
        m_settings = ScreenSaverSettings();
        m_hasData  = false;
    }

    bool                        HasData()   const { return m_hasData;  }
    const ScreenSaverSettings & GetStored() const { return m_settings; }

private:
    ScreenSaverSettings m_settings;
    bool                m_hasData = false;
};


