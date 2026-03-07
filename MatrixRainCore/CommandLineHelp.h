#pragma once

#include "UsageDialog.h"
#include "UsageText.h"





class CommandLineHelp
{
public:
    // Orchestration — top-level entry point for /?/-? handling
    static HRESULT DisplayCommandLineHelp (wchar_t switchPrefix);
};

