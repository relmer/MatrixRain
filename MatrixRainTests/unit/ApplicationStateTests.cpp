#include "pch.h"

#include "..\..\MatrixRainCore\ApplicationState.h"
#include "..\..\MatrixRainCore\RegistrySettingsProvider.h"





namespace MatrixRainTests
{
    TEST_CLASS (ApplicationStateTests)
    {
    private:
        static constexpr LPCWSTR TEST_REGISTRY_KEY_PATH = L"Software\\relmer\\MatrixRain_AppStateTest";
        
        static void DeleteTestRegistryKey()
        {
            RegDeleteTreeW (HKEY_CURRENT_USER, TEST_REGISTRY_KEY_PATH);
        }
        
    public:
        TEST_CLASS_INITIALIZE (Initialize)
        {
            RegistrySettingsProvider::SetRegistryKeyPath (TEST_REGISTRY_KEY_PATH);
        }
        
        TEST_CLASS_CLEANUP (Cleanup)
        {
            DeleteTestRegistryKey();
            RegistrySettingsProvider::ResetRegistryKeyPath();
        }
        
        TEST_METHOD_INITIALIZE (MethodSetup)
        {
            // Ensure clean registry state before each test
            DeleteTestRegistryKey();
        }
        
        // T116: Test ApplicationState display mode initialization to Fullscreen
        TEST_METHOD (TestApplicationStateInitializesToFullscreen)
        {
            ApplicationState appState;
            appState.Initialize (nullptr);

            DisplayMode mode = appState.GetDisplayMode ();
            Assert::AreEqual (static_cast<int>(DisplayMode::Fullscreen), 
                              static_cast<int>(mode),
                              L"ApplicationState should initialize to Fullscreen display mode");
        }





        // T117: Test ApplicationState ToggleDisplayMode transition Fullscreen→Windowed
        TEST_METHOD (TestToggleDisplayModeFullscreenToWindowed)
        {
            ApplicationState appState;
            appState.Initialize (nullptr);

            // Verify starts in fullscreen
            Assert::AreEqual (static_cast<int>(DisplayMode::Fullscreen),
                              static_cast<int>(appState.GetDisplayMode ()),
                              L"Should start in Fullscreen mode");

            // Toggle to windowed
            appState.ToggleDisplayMode ();

            Assert::AreEqual (static_cast<int>(DisplayMode::Windowed),
                              static_cast<int>(appState.GetDisplayMode ()),
                              L"Should transition to Windowed mode after toggle");
        }





        // T118: Test ApplicationState ToggleDisplayMode transition Windowed→Fullscreen
        TEST_METHOD (TestToggleDisplayModeWindowedToFullscreen)
        {
            ApplicationState appState;
            appState.Initialize (nullptr);

            // Toggle to windowed
            appState.ToggleDisplayMode ();
            Assert::AreEqual (static_cast<int>(DisplayMode::Windowed),
                              static_cast<int>(appState.GetDisplayMode ()),
                              L"Should be in Windowed after first toggle");

            // Toggle back to fullscreen
            appState.ToggleDisplayMode ();
            Assert::AreEqual (static_cast<int>(DisplayMode::Fullscreen),
                              static_cast<int>(appState.GetDisplayMode ()),
                              L"Should transition back to Fullscreen mode after second toggle");
        }
    };
}  // namespace MatrixRainTests

