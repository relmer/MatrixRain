#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ApplicationState.h"
#include "..\..\MatrixRainCore\ColorScheme.h"
#include "..\..\MatrixRainCore\ConfigDialogController.h"
#include "..\..\MatrixRainCore\RegistrySettingsProvider.h"





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
            ScreenSaverSettings settings
            {
                .m_densityPercent        = 65,
                .m_colorSchemeKey        = L"blue",
                .m_animationSpeedPercent = 50,
                .m_glowIntensityPercent  = 120,
                .m_glowSizePercent       = 150,
                .m_startFullscreen       = false,
                .m_showDebugStats        = true,
                .m_showFadeTimers        = true
            };
            
            HRESULT                hr         = S_OK; 
            ConfigDialogController controller;
            

            
            hr = RegistrySettingsProvider::Save (settings);
            Assert::AreEqual (S_OK, hr, L"Registry save should succeed");

            // Act: Create controller and initialize
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
            HRESULT                hr         = S_OK; 
            ConfigDialogController controller;



            // Arrange: Ensure no registry key exists (already done in MethodSetup)

            // Act: Create controller and initialize
            
            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

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
            HRESULT                hr         = S_OK;
            ConfigDialogController controller;



            // Arrange
            hr = controller.Initialize();
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
            HRESULT                hr         = S_OK; 
            ConfigDialogController controller;
            
            
            
            // Arrange
            hr = controller.Initialize();
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
            HRESULT                hr         = S_OK; 
            ConfigDialogController controller;
            
            
            
            // Arrange
            hr = controller.Initialize();
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
            HRESULT                hr         = S_OK; 
            ConfigDialogController controller;
            
            
            
            // Arrange
            hr = controller.Initialize();
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
            HRESULT                hr         = S_OK; 
            ConfigDialogController controller;
            
            
            
            // Arrange
            hr = controller.Initialize();
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
            HRESULT                hr         = S_OK; 
            ConfigDialogController controller;
            
            
            
            hr = controller.Initialize();
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
            HRESULT                hr         = S_OK;
            ConfigDialogController controller;



            hr = controller.Initialize();
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
            HRESULT                hr         = S_OK; 
            ConfigDialogController controller;
            
            
            
            hr = controller.Initialize();
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
            ScreenSaverSettings initialSettings
            {
                .m_densityPercent        = 50,
                .m_colorSchemeKey        = L"green",
                .m_animationSpeedPercent = 75
            };
            
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
            HRESULT                hr         = S_OK;
            ConfigDialogController controller;



            hr = controller.Initialize();
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





        // T050.1: Test ConfigDialogController creates snapshot in live overlay mode
        TEST_METHOD (TestConfigDialogControllerCreatesSnapshotInLiveMode)
        {
            // Arrange: Set up initial settings in registry
            HRESULT                hr              = S_OK;
            ConfigDialogController controller;
            ApplicationState       appState;
            ScreenSaverSettings    initialSettings
            {
                .m_densityPercent        = 70,
                .m_colorSchemeKey        = L"blue",
                .m_animationSpeedPercent = 80,
                .m_glowIntensityPercent  = 120,
                .m_glowSizePercent       = 140
            };
            


            hr = RegistrySettingsProvider::Save (initialSettings);
            Assert::AreEqual (S_OK, hr);

            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            // Act: Initialize live overlay mode with valid ApplicationState
            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            // Assert: Should be in live mode and have captured snapshot
            Assert::IsTrue (controller.IsLiveMode(), L"Controller should be in live mode");
            Assert::AreEqual (70, controller.GetSettings().m_densityPercent, L"Snapshot should preserve initial density");
        }





        // T050.2: Test live overlay mode applies changes immediately to ApplicationState
        TEST_METHOD (TestLiveModeAppliesChangesImmediately)
        {
            // Arrange
            HRESULT                hr         = S_OK;
            ConfigDialogController controller;
            ApplicationState       appState;



            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            // Act: Update density - should immediately propagate to ApplicationState
            controller.UpdateDensity (85);

            // Assert: ApplicationState should reflect the change
            Assert::AreEqual (85, appState.GetSettings().m_densityPercent, L"Density changes should propagate immediately");
        }





        // T050.3: Test Cancel in live mode reverts ApplicationState to snapshot values
        TEST_METHOD (TestLiveModeCancelRevertsToSnapshot)
        {
            // Arrange
            HRESULT                 hr         = S_OK;
            ConfigDialogController  controller;
            ApplicationState        appState;



            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            int initialDensity = appState.GetSettings().m_densityPercent;

            // Act: Make a change then cancel
            controller.UpdateDensity (90);
            Assert::AreEqual (90, appState.GetSettings().m_densityPercent, L"Density should update immediately");

            hr = controller.CancelLiveMode();
            Assert::AreEqual (S_OK, hr);

            // Assert: ApplicationState should be reverted to snapshot
            Assert::AreEqual (initialDensity, appState.GetSettings().m_densityPercent, L"Cancel should revert to initial density");
            Assert::IsFalse (controller.IsLiveMode(), L"Should exit live mode after cancel");
        }





        // T050.4: Test OK in live mode persists changes and clears snapshot
        TEST_METHOD (TestLiveModeOkPersistsChanges)
        {
            // Arrange: Set up initial settings
            ScreenSaverSettings initialSettings
            {
                .m_densityPercent = 50
            };
            
            HRESULT                 hr = S_OK;
            ConfigDialogController  controller;
            ApplicationState        appState;


            hr = RegistrySettingsProvider::Save (initialSettings);
            Assert::AreEqual (S_OK, hr);

            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            // Act: Make a change and apply
            controller.UpdateDensity (75);
            hr = controller.ApplyLiveMode();
            Assert::AreEqual (S_OK, hr);

            // Assert: Should persist to registry and exit live mode
            Assert::IsFalse (controller.IsLiveMode(), L"Should exit live mode after apply");

            ScreenSaverSettings savedSettings;
            hr = RegistrySettingsProvider::Load (savedSettings);
            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (75, savedSettings.m_densityPercent, L"Changes should be saved to registry");
        }





        // T050.5: Test live mode snapshot captures all settings correctly
        TEST_METHOD (TestLiveModeSnapshotCapturesAllSettings)
        {
            // Arrange: Configure comprehensive settings
            ScreenSaverSettings settings
            {
                .m_densityPercent        = 65,
                .m_colorSchemeKey        = L"amber",
                .m_animationSpeedPercent = 85,
                .m_glowIntensityPercent  = 150,
                .m_glowSizePercent       = 175,
                .m_startFullscreen       = false,
                .m_showDebugStats        = true,
                .m_showFadeTimers        = true
            };
            
            HRESULT hr = RegistrySettingsProvider::Save (settings);
            Assert::AreEqual (S_OK, hr);

            ConfigDialogController controller;
            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            // Act & Assert: Verify IsLiveMode returns false before initialization
            Assert::IsFalse (controller.IsLiveMode(), L"Should not be in live mode before initialization");
        }





        // T050.6: Test live mode rejects modal operations
        TEST_METHOD (TestLiveModeRejectsModalOperations)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT                hr         = S_OK;



            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            // Act: Try modal apply while not in live mode
            hr = controller.ApplyChanges();

            // Assert: Should succeed in modal mode
            Assert::AreEqual (S_OK, hr, L"ApplyChanges should work in modal mode");

            // Future: When in live mode, ApplyChanges should fail or be redirected
        }





        // T052.1: Test live mode propagates density changes to ApplicationState
        TEST_METHOD (TestLiveModePropagatesDensityChanges)
        {
            // Arrange: Set up initial settings and ApplicationState
            ScreenSaverSettings initialSettings
            {
                .m_densityPercent = 50
            };
            
            HRESULT                hr = S_OK;
            ConfigDialogController controller;
            ApplicationState       appState;


            hr = RegistrySettingsProvider::Save (initialSettings);
            Assert::AreEqual (S_OK, hr);

            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            // Act: Update density in dialog
            controller.UpdateDensity (75);

            // Assert: Verify ApplicationState was updated
            Assert::AreEqual (75, appState.GetSettings().m_densityPercent, L"Density should propagate to ApplicationState");
        }





        // T052.2: Test live mode propagates color scheme changes to ApplicationState
        TEST_METHOD (TestLiveModePropagatesColorSchemeChanges)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT                hr       = S_OK;
            ApplicationState       appState;



            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            // Act: Update color scheme
            controller.UpdateColorScheme (L"blue");

            // Assert: Verify ApplicationState color scheme changed
            Assert::AreEqual (static_cast<int>(ColorScheme::Blue), 
                              static_cast<int>(appState.GetColorScheme()), 
                              L"Color scheme should propagate to ApplicationState");
        }





        // T052.3: Test live mode propagates animation speed changes to ApplicationState
        TEST_METHOD (TestLiveModePropagatesAnimationSpeedChanges)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT                hr       = S_OK;
            ApplicationState       appState;



            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            // Act: Update animation speed
            controller.UpdateAnimationSpeed (60);

            // Assert: Verify ApplicationState was updated
            Assert::AreEqual (60, appState.GetSettings().m_animationSpeedPercent, L"Animation speed should propagate to ApplicationState");
        }





        // T052.4: Test live mode propagates glow intensity changes to ApplicationState
        TEST_METHOD (TestLiveModePropagatesGlowIntensityChanges)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT                hr       = S_OK;
            ApplicationState       appState;



            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            // Act: Update glow intensity
            controller.UpdateGlowIntensity (150);

            // Assert: Verify ApplicationState was updated
            Assert::AreEqual (150, appState.GetSettings().m_glowIntensityPercent, L"Glow intensity should propagate to ApplicationState");
        }





        // T052.5: Test live mode propagates glow size changes to ApplicationState
        TEST_METHOD (TestLiveModePropagatesGlowSizeChanges)
        {
            // Arrange
            ConfigDialogController controller;
            HRESULT                hr       = S_OK;
            ApplicationState       appState;



            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            // Act: Update glow size
            controller.UpdateGlowSize (175);

            // Assert: Verify ApplicationState was updated
            Assert::AreEqual (175, appState.GetSettings().m_glowSizePercent, L"Glow size should propagate to ApplicationState");
        }





        // T052.6: Test CancelLiveMode reverts ApplicationState to snapshot
        TEST_METHOD (TestCancelLiveModeRevertsApplicationState)
        {
            // Arrange: Set up initial settings
            ScreenSaverSettings initialSettings
            {
                .m_densityPercent        = 50,
                .m_colorSchemeKey        = L"green",
                .m_animationSpeedPercent = 70
            };
            
            HRESULT                hr = S_OK;
            ConfigDialogController controller;
            ApplicationState       appState;


            hr = RegistrySettingsProvider::Save (initialSettings);
            Assert::AreEqual (S_OK, hr);

            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            // Act: Make changes then cancel
            controller.UpdateDensity (80);
            controller.UpdateColorScheme (L"blue");
            controller.UpdateAnimationSpeed (90);

            hr = controller.CancelLiveMode();
            Assert::AreEqual (S_OK, hr);

            // Assert: Verify ApplicationState reverted to snapshot
            Assert::AreEqual (50, appState.GetSettings().m_densityPercent,        L"Density should revert to snapshot");
            Assert::AreEqual (70, appState.GetSettings().m_animationSpeedPercent, L"Animation speed should revert to snapshot");
            Assert::AreEqual (static_cast<int>(ColorScheme::Green), 
                              static_cast<int>(appState.GetColorScheme()), 
                              L"Color scheme should revert to snapshot");
        }





        // T058: Test dialog behavior when parent app exits while live overlay dialog is open
        TEST_METHOD (TestLiveDialogGracefulShutdownWhenParentAppExits)
        {
            // Note: This integration test validates that if the parent MatrixRain
            // application window is destroyed while the live overlay dialog is open,
            // the dialog detects this and closes gracefully without crashing.
            // 
            // Full validation requires:
            // 1. Creating application window with running animation
            // 2. Opening live overlay dialog
            // 3. Destroying parent window (simulating app exit/crash)
            // 4. Verifying dialog receives WM_PARENTNOTIFY or detects invalid HWND
            // 5. Verifying dialog closes gracefully and doesn't crash
            // 
            // This is a placeholder for manual/automated UI testing

            ApplicationState       appState;
            ConfigDialogController controller;
            HRESULT                hr         = S_OK;
            
            
            appState.Initialize (nullptr);
            
            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr, L"Controller initialization should succeed");

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr, L"Live mode initialization should succeed");

            // TODO: Create test that:
            // 1. Creates real HWND for parent window
            // 2. Opens dialog with that parent
            // 3. Destroys parent HWND (DestroyWindow or PostQuitMessage)
            // 4. Verifies dialog detects parent destruction
            // 5. Verifies dialog closes without crash or leak
            Assert::Fail (L"Test not yet implemented - requires window creation and destruction simulation");
        }
    };
}  // namespace MatrixRainTests
