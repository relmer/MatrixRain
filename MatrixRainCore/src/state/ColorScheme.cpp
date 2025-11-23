#include "pch.h"
#include "MatrixRain/ColorScheme.h"

namespace MatrixRain
{
    ColorScheme GetNextColorScheme(ColorScheme current)
    {
        switch (current)
        {
        case ColorScheme::Green:
            return ColorScheme::Blue;
        case ColorScheme::Blue:
            return ColorScheme::Red;
        case ColorScheme::Red:
            return ColorScheme::Amber;
        case ColorScheme::Amber:
            return ColorScheme::Green;
        default:
            return ColorScheme::Green;
        }
    }

    Color4 GetColorRGB(ColorScheme scheme)
    {
        switch (scheme)
        {
        case ColorScheme::Green:
            return Color4(0.0f, 1.0f, 100.0f / 255.0f);  // (0, 255, 100)
        case ColorScheme::Blue:
            return Color4(0.0f, 100.0f / 255.0f, 1.0f);  // (0, 100, 255)
        case ColorScheme::Red:
            return Color4(1.0f, 50.0f / 255.0f, 50.0f / 255.0f);  // (255, 50, 50)
        case ColorScheme::Amber:
            return Color4(1.0f, 191.0f / 255.0f, 0.0f);  // (255, 191, 0)
        default:
            return Color4(0.0f, 1.0f, 100.0f / 255.0f);  // Default to green
        }
    }
}
