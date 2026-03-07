#pragma once

#include "ScreenSaverSettings.h"





////////////////////////////////////////////////////////////////////////////////
//
//  ISettingsProvider
//
//  Abstract interface for loading and saving screensaver settings.
//  Allows production code to use the registry and tests to use an in-memory
//  implementation without touching the Windows registry.
//
////////////////////////////////////////////////////////////////////////////////

class ISettingsProvider
{
public:
    virtual ~ISettingsProvider() = default;

    virtual HRESULT Load (ScreenSaverSettings & settings) = 0;
    virtual HRESULT Save (const ScreenSaverSettings & settings) = 0;
};


