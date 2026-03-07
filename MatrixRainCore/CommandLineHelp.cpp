#include "pch.h"

#include "CommandLineHelp.h"

#include "RegistrySettingsProvider.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CommandLineHelp::DisplayCommandLineHelp
//
//  Top-level orchestration for /? and -? help display.
//  Creates a UsageDialog with matrix rain reveal animation.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CommandLineHelp::DisplayCommandLineHelp (wchar_t switchPrefix)
{
    HRESULT                  hr               = S_OK;
    UsageText                usage              (switchPrefix);
    RegistrySettingsProvider settingsProvider;
    UsageDialog               dialog             (usage, settingsProvider);


    
    hr = dialog.Show();
    CHR (hr);

    

Error:
    return hr;
}
