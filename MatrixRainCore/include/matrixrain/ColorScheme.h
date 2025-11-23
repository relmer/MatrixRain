#pragma once
#include "matrixrain/Math.h"

namespace MatrixRain
{
    /// <summary>
    /// Color schemes for Matrix rain effect.
    /// </summary>
    enum class ColorScheme
    {
        Green,   // Classic Matrix green (0, 255, 100)
        Blue,    // Cool blue (0, 100, 255)
        Red,     // Danger red (255, 50, 50)
        Amber    // Warm amber (255, 191, 0)
    };

    /// <summary>
    /// Get the next color scheme in the cycling sequence.
    /// Green → Blue → Red → Amber → Green
    /// </summary>
    /// <param name="current">Current color scheme</param>
    /// <returns>Next color scheme in sequence</returns>
    ColorScheme GetNextColorScheme(ColorScheme current);

    /// <summary>
    /// Get RGB color values for a color scheme.
    /// </summary>
    /// <param name="scheme">Color scheme to query</param>
    /// <returns>Color4 with RGB values (0-1 range)</returns>
    Color4 GetColorRGB(ColorScheme scheme);
}
