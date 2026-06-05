#include "pch.h"

#include "ScanlineStyleMapping.h"





float ScanlineLineCount (int style) noexcept
{
    const int clamped = std::clamp (style, 1, 100);


    return 1000.0f * std::pow (0.15f, static_cast<float> (clamped) / 100.0f);
}
