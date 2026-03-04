#include "pch.h"

#include "CommandLineHelp.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CommandLineHelp::DisplayCommandLineHelp
//
//  Top-level orchestration for /? and -? help display.
//  Shows a MessageBox with the formatted usage text.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CommandLineHelp::DisplayCommandLineHelp (wchar_t switchPrefix)
{
    UsageText usage (switchPrefix);



    MessageBoxW (nullptr, usage.GetPlainText().c_str(), L"MatrixRain \u2014 Help", MB_OK | MB_ICONINFORMATION);

    return S_OK;
}
