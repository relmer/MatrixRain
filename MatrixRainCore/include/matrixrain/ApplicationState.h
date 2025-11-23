#pragma once

namespace MatrixRain
{
    /// <summary>
    /// Display mode enumeration for windowed vs fullscreen
    /// </summary>
    enum class DisplayMode
    {
        Windowed,
        Fullscreen
    };

    /// <summary>
    /// Manages application-wide state including display mode.
    /// Coordinates between different systems to maintain consistent state.
    /// </summary>
    class ApplicationState
    {
    public:
        ApplicationState();

        /// <summary>
        /// Initialize application state with default values.
        /// Starts in Windowed display mode.
        /// </summary>
        void Initialize();

        /// <summary>
        /// Toggle between Windowed and Fullscreen display modes.
        /// </summary>
        void ToggleDisplayMode();

        // Accessors
        DisplayMode GetDisplayMode() const { return m_displayMode; }
        void SetDisplayMode(DisplayMode mode) { m_displayMode = mode; }

    private:
        DisplayMode m_displayMode;  // Current display mode
    };
}
