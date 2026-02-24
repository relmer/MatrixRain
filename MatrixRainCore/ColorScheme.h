#pragma once





#include "Math.h"





/// <summary>
/// Color schemes for Matrix rain effect.
/// Values 0-3 are static colors that map directly to color table indices.
/// </summary>
enum class ColorScheme
{
    Green              = 0,                     // Classic Matrix green (0, 255, 100)
    Blue               = 1,                     // Cool blue (0, 100, 255)
    Red                = 2,                     // Danger red (255, 50, 50)
    Amber              = 3,                     // Warm amber (255, 191, 0)
    __StaticColorCount,                         // Number of static colors (not including ColorCycle)
    ColorCycle         = __StaticColorCount     // Continuously cycles through all colors smoothly
};

/// <summary>
/// Get the next color scheme in the cycling sequence.
/// Green → Blue → Red → Amber → ColorCycle → Green
/// </summary>
/// <param name="current">Current color scheme</param>
/// <returns>Next color scheme in sequence</returns>
ColorScheme GetNextColorScheme (ColorScheme current);

/// <summary>
/// Get RGB color values for a color scheme.
/// For ColorCycle mode, smoothly transitions through all colors based on elapsed time.
/// </summary>
/// <param name="scheme">Color scheme to query</param>
/// <param name="elapsedTime">Elapsed time in seconds (used for ColorCycle mode)</param>
/// <returns>Color4 with RGB values (0-1 range)</returns>
Color4 GetColorRGB (ColorScheme scheme, float elapsedTime = 0.0f);

/// <summary>
/// Parse a color scheme key string to its enum value.
/// Returns ColorScheme::Green for unrecognized keys.
/// </summary>
/// <param name="key">Color scheme key (green, blue, red, amber, cycle)</param>
/// <returns>Corresponding ColorScheme enum value</returns>
ColorScheme ParseColorSchemeKey (const std::wstring & key);

/// <summary>
/// Convert a ColorScheme enum value to its string key.
/// </summary>
/// <param name="scheme">Color scheme enum value</param>
/// <returns>String key (e.g., L"green", L"blue")</returns>
std::wstring ColorSchemeToKey (ColorScheme scheme);

/// <summary>
/// Check if a color scheme key string is valid.
/// </summary>
/// <param name="key">Color scheme key to validate (case-insensitive)</param>
/// <returns>True if the key maps to a known color scheme</returns>
bool IsValidColorSchemeKey (const std::wstring & key);




