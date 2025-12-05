#include "pch.h"

#include "../../MatrixRainCore/ConfigDialogController.h"
#include "../../MatrixRainCore/RegistrySettingsProvider.h"





namespace MatrixRainTests
{
    TEST_CLASS (ConfigDialogControllerTests)
    {
    private:
        static constexpr LPCWSTR TEST_REGISTRY_KEY_PATH = L"Software\\relmer\\MatrixRain_ConfigDialogTest";
        
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





        // T019.1: Test ConfigDialogController loads existing settings on initialization
        TEST_METHOD (TestConfigDialogControllerLoadsExistingSettings)
        {
            // Arrange: Save known settings to registry
            ScreenSaverSettings settings;
            settings.m_densityPercent        = 65;
            settings.m_colorSchemeKey        = L"blue";
            settings.m_animationSpeedPercent = 50;
            settings.m_glowIntensityPercent  = 120;
            settings.m_glowSizePercent       = 150;
            settings.m_startFullscreen       = false;
            settings.m_showDebugStats        = true;
            settings.m_showFadeTimers        = true;
            
            HRESULT hr = RegistrySettingsProvider::Save (settings);
            Assert::AreEqual (S_OK, hr, L"Registry save should succeed");

            // Act: Create controller and initialize
            ConfigDialogController controller;
            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr, L"Controller initialization should succeed");

            // Assert: Verify loaded settings match saved values
            const ScreenSaverSettings & loaded = controller.GetSettings();
            Assert::AreEqual (65,       loaded.m_densityPercent,        L"Density should match saved value");
            Assert::AreEqual (L"blue",  loaded.m_colorSchemeKey.c_str(), L"Color scheme should match saved value");
            Assert::AreEqual (50,       loaded.m_animationSpeedPercent, L"Animation speed should match saved value");
            Assert::AreEqual (120,      loaded.m_glowIntensityPercent,  L"Glow intensity should match saved value");
            Assert::AreEqual (150,      loaded.m_glowSizePercent,       L"Glow size should match saved value");
            Assert::IsFalse  (loaded.m_startFullscreen,                 L"Start fullscreen should match saved value");
            Assert::IsTrue   (loaded.m_showDebugStats,                  L"Show debug stats should match saved value");
            Assert::IsTrue   (loaded.m_showFadeTimers,                  L"Show fade timers should match saved value");
        }





        // T019.2: Test ConfigDialogController uses defaults when no registry key exists
        TEST_METHOD (TestConfigDialogControllerUsesDefaultsWhenNoRegistry)
        {
            // Arrange: Ensure no registry key exists (already done in MethodSetup)

            // Act: Create controller and initialize
            ConfigDialogController controller;
            HRESULT hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr, L"Controller initialization should succeed with defaults");

            // Assert: Verify settings use defaults
            const ScreenSaverSettings & loaded = controller.GetSettings();
            Assert::AreEqual (ScreenSaverSettings::DEFAULT_DENSITY_PERCENT,         loaded.m_densityPercent,        L"Should use default density");
            Assert::AreEqual (ScreenSaverSettings::DEFAULT_ANIMATION_SPEED_PERCENT, loaded.m_animationSpeedPercent, L"Should use default animation speed");
            Assert::AreEqual (ScreenSaverSettings::DEFAULT_GLOW_INTENSITY_PERCENT,  loaded.m_glowIntensityPercent,  L"Should use default glow intensity");
            Assert::AreEqual (ScreenSaverSettings::DEFAULT_GLOW_SIZE_PERCENT,       loaded.m_glowSizePercent,       L"Should use default glow size");
            Assert::IsTrue   (loaded.m_startFullscreen,                             L"Should default to fullscreen");
            Assert::IsFalse  (loaded.m_showDebugStats,                              L"Should default to no debug stats");
            Assert::IsFalse  (loaded.m_showFadeTimers,                              L"Should default to no fade timers");
        }





        // T019.3: Test ConfigDialogController validates and clamps density slider updates
        TEST_METHOD (TestConfigDialogControllerClampsDensitySliderUpdates)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            // Act & Assert: Test valid range
            controller.UpdateDensity (50);
            Assert::AreEqual (50, controller.GetSettings().m_densityPercent, L"Valid density should be accepted");

            // Act & Assert: Test clamping below minimum
            controller.UpdateDensity (-10);
            Assert::AreEqual (ScreenSaverSettings::MIN_DENSITY_PERCENT, 
                              controller.GetSettings().m_densityPercent, 
                              L"Density below minimum should be clamped to 0");

            // Act & Assert: Test clamping above maximum
            controller.UpdateDensity (150);
            Assert::AreEqual (ScreenSaverSettings::MAX_DENSITY_PERCENT, 
                              controller.GetSettings().m_densityPercent, 
                              L"Density above maximum should be clamped to 100");
        }





        // T019.4: Test ConfigDialogController validates and clamps animation speed updates
        TEST_METHOD (TestConfigDialogControllerClampsAnimationSpeedUpdates)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            // Act & Assert: Test valid range
            controller.UpdateAnimationSpeed (60);
            Assert::AreEqual (60, controller.GetSettings().m_animationSpeedPercent, L"Valid animation speed should be accepted");

            // Act & Assert: Test clamping below minimum
            controller.UpdateAnimationSpeed (0);
            Assert::AreEqual (ScreenSaverSettings::MIN_ANIMATION_SPEED_PERCENT, 
                              controller.GetSettings().m_animationSpeedPercent, 
                              L"Animation speed below minimum should be clamped to 1");

            // Act & Assert: Test clamping above maximum
            controller.UpdateAnimationSpeed (200);
            Assert::AreEqual (ScreenSaverSettings::MAX_ANIMATION_SPEED_PERCENT, 
                              controller.GetSettings().m_animationSpeedPercent, 
                              L"Animation speed above maximum should be clamped to 100");
        }





        // T019.5: Test ConfigDialogController validates and clamps glow intensity updates
        TEST_METHOD (TestConfigDialogControllerClampsGlowIntensityUpdates)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            // Act & Assert: Test valid range
            controller.UpdateGlowIntensity (150);
            Assert::AreEqual (150, controller.GetSettings().m_glowIntensityPercent, L"Valid glow intensity should be accepted");

            // Act & Assert: Test clamping below minimum
            controller.UpdateGlowIntensity (-50);
            Assert::AreEqual (ScreenSaverSettings::MIN_GLOW_INTENSITY_PERCENT, 
                              controller.GetSettings().m_glowIntensityPercent, 
                              L"Glow intensity below minimum should be clamped to 0");

            // Act & Assert: Test clamping above maximum
            controller.UpdateGlowIntensity (300);
            Assert::AreEqual (ScreenSaverSettings::MAX_GLOW_INTENSITY_PERCENT, 
                              controller.GetSettings().m_glowIntensityPercent, 
                              L"Glow intensity above maximum should be clamped to 200");
        }





        // T019.6: Test ConfigDialogController validates and clamps glow size updates
        TEST_METHOD (TestConfigDialogControllerClampsGlowSizeUpdates)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            // Act & Assert: Test valid range
            controller.UpdateGlowSize (175);
            Assert::AreEqual (175, controller.GetSettings().m_glowSizePercent, L"Valid glow size should be accepted");

            // Act & Assert: Test clamping below minimum
            controller.UpdateGlowSize (25);
            Assert::AreEqual (ScreenSaverSettings::MIN_GLOW_SIZE_PERCENT, 
                              controller.GetSettings().m_glowSizePercent, 
                              L"Glow size below minimum should be clamped to 50");

            // Act & Assert: Test clamping above maximum
            controller.UpdateGlowSize (250);
            Assert::AreEqual (ScreenSaverSettings::MAX_GLOW_SIZE_PERCENT, 
                              controller.GetSettings().m_glowSizePercent, 
                              L"Glow size above maximum should be clamped to 200");
        }





        // T019.7: Test ConfigDialogController updates color scheme selection
        TEST_METHOD (TestConfigDialogControllerUpdatesColorScheme)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            // Act & Assert: Test each color scheme
            controller.UpdateColorScheme (L"green");
            Assert::AreEqual (L"green", controller.GetSettings().m_colorSchemeKey.c_str(), L"Should update to green");

            controller.UpdateColorScheme (L"blue");
            Assert::AreEqual (L"blue", controller.GetSettings().m_colorSchemeKey.c_str(), L"Should update to blue");

            controller.UpdateColorScheme (L"red");
            Assert::AreEqual (L"red", controller.GetSettings().m_colorSchemeKey.c_str(), L"Should update to red");

            controller.UpdateColorScheme (L"amber");
            Assert::AreEqual (L"amber", controller.GetSettings().m_colorSchemeKey.c_str(), L"Should update to amber");

            controller.UpdateColorScheme (L"cycle");
            Assert::AreEqual (L"cycle", controller.GetSettings().m_colorSchemeKey.c_str(), L"Should update to cycle");
        }





        // T019.8: Test ConfigDialogController rejects invalid color scheme keys
        TEST_METHOD (TestConfigDialogControllerRejectsInvalidColorScheme)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            std::wstring originalScheme = controller.GetSettings().m_colorSchemeKey;

            // Act: Try to set invalid color scheme
            controller.UpdateColorScheme (L"invalid_scheme");

            // Assert: Should reject and keep original
            Assert::AreEqual (originalScheme.c_str(), 
                              controller.GetSettings().m_colorSchemeKey.c_str(), 
                              L"Invalid color scheme should be rejected");
        }





        // T019.9: Test ConfigDialogController updates boolean flags
        TEST_METHOD (TestConfigDialogControllerUpdatesBooleanFlags)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            // Act & Assert: Test fullscreen toggle
            controller.UpdateStartFullscreen (true);
            Assert::IsTrue (controller.GetSettings().m_startFullscreen, L"Should set start fullscreen to true");

            controller.UpdateStartFullscreen (false);
            Assert::IsFalse (controller.GetSettings().m_startFullscreen, L"Should set start fullscreen to false");

            // Act & Assert: Test debug stats toggle
            controller.UpdateShowDebugStats (true);
            Assert::IsTrue (controller.GetSettings().m_showDebugStats, L"Should set show debug stats to true");

            controller.UpdateShowDebugStats (false);
            Assert::IsFalse (controller.GetSettings().m_showDebugStats, L"Should set show debug stats to false");

            // Act & Assert: Test fade timers toggle
            controller.UpdateShowFadeTimers (true);
            Assert::IsTrue (controller.GetSettings().m_showFadeTimers, L"Should set show fade timers to true");

            controller.UpdateShowFadeTimers (false);
            Assert::IsFalse (controller.GetSettings().m_showFadeTimers, L"Should set show fade timers to false");
        }





        // T019.10: Test ConfigDialogController saves settings on ApplyChanges
        TEST_METHOD (TestConfigDialogControllerSavesOnApply)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            // Modify settings
            controller.UpdateDensity (80);
            controller.UpdateColorScheme (L"red");
            controller.UpdateAnimationSpeed (90);
            controller.UpdateGlowIntensity (110);
            controller.UpdateGlowSize (130);
            controller.UpdateStartFullscreen (false);

            // Act: Apply changes
            hr = controller.ApplyChanges();
            Assert::AreEqual (S_OK, hr, L"ApplyChanges should succeed");

            // Assert: Verify settings were saved to registry
            ScreenSaverSettings loadedSettings;
            hr = RegistrySettingsProvider::Load (loadedSettings);
            Assert::AreEqual (S_OK, hr, L"Registry load should succeed");

            Assert::AreEqual (80,     loadedSettings.m_densityPercent,        L"Density should be saved");
            Assert::AreEqual (L"red", loadedSettings.m_colorSchemeKey.c_str(), L"Color scheme should be saved");
            Assert::AreEqual (90,     loadedSettings.m_animationSpeedPercent, L"Animation speed should be saved");
            Assert::AreEqual (110,    loadedSettings.m_glowIntensityPercent,  L"Glow intensity should be saved");
            Assert::AreEqual (130,    loadedSettings.m_glowSizePercent,       L"Glow size should be saved");
            Assert::IsFalse  (loadedSettings.m_startFullscreen,               L"Start fullscreen should be saved");
        }





        // T019.11: Test ConfigDialogController discards changes on Cancel
        TEST_METHOD (TestConfigDialogControllerDiscardsChangesOnCancel)
        {
            // Arrange: Save initial settings to registry
            ScreenSaverSettings initialSettings;
            initialSettings.m_densityPercent        = 50;
            initialSettings.m_colorSchemeKey        = L"green";
            initialSettings.m_animationSpeedPercent = 75;
            
            HRESULT hr = RegistrySettingsProvider::Save (initialSettings);
            Assert::AreEqual (S_OK, hr);

            ConfigDialogController controller;
            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            // Modify settings but don't apply
            controller.UpdateDensity (90);
            controller.UpdateColorScheme (L"blue");
            controller.UpdateAnimationSpeed (50);

            // Act: Cancel changes
            controller.CancelChanges();

            // Assert: Verify registry still contains original settings
            ScreenSaverSettings loadedSettings;
            hr = RegistrySettingsProvider::Load (loadedSettings);
            Assert::AreEqual (S_OK, hr);

            Assert::AreEqual (50,       loadedSettings.m_densityPercent,        L"Density should remain unchanged");
            Assert::AreEqual (L"green", loadedSettings.m_colorSchemeKey.c_str(), L"Color scheme should remain unchanged");
            Assert::AreEqual (75,       loadedSettings.m_animationSpeedPercent, L"Animation speed should remain unchanged");
        }





        // T019.12: Test ConfigDialogController reloads settings on ResetToDefaults
        TEST_METHOD (TestConfigDialogControllerResetsToDefaults)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            // Modify settings
            controller.UpdateDensity (25);
            controller.UpdateColorScheme (L"red");
            controller.UpdateAnimationSpeed (30);
            controller.UpdateGlowIntensity (180);
            controller.UpdateStartFullscreen (false);

            // Act: Reset to defaults
            controller.ResetToDefaults();

            // Assert: Verify settings are reset to defaults
            const ScreenSaverSettings & settings = controller.GetSettings();
            Assert::AreEqual (ScreenSaverSettings::DEFAULT_DENSITY_PERCENT,         settings.m_densityPercent,        L"Density should reset to default");
            Assert::AreEqual (ScreenSaverSettings::DEFAULT_ANIMATION_SPEED_PERCENT, settings.m_animationSpeedPercent, L"Animation speed should reset to default");
            Assert::AreEqual (ScreenSaverSettings::DEFAULT_GLOW_INTENSITY_PERCENT,  settings.m_glowIntensityPercent,  L"Glow intensity should reset to default");
            Assert::AreEqual (ScreenSaverSettings::DEFAULT_GLOW_SIZE_PERCENT,       settings.m_glowSizePercent,       L"Glow size should reset to default");
            Assert::IsTrue   (settings.m_startFullscreen,                           L"Start fullscreen should reset to default");
            Assert::IsFalse  (settings.m_showDebugStats,                            L"Show debug stats should reset to default");
            Assert::IsFalse  (settings.m_showFadeTimers,                            L"Show fade timers should reset to default");
        }
    };
}  // namespace MatrixRainTests
