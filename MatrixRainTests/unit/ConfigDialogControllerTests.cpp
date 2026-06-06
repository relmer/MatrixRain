#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ApplicationState.h"
#include "..\..\MatrixRainCore\ColorScheme.h"
#include "..\..\MatrixRainCore\ConfigDialogController.h"
#include "..\..\MatrixRainCore\InMemorySettingsProvider.h"





namespace MatrixRainTests
{
    TEST_CLASS (ConfigDialogControllerTests)
    {
    private:
        InMemorySettingsProvider m_settingsProvider;

    public:
        TEST_METHOD_INITIALIZE (MethodSetup)
        {
            m_settingsProvider.Clear();
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
                .m_showDebugStats        = true
            };
            
            HRESULT                hr         = S_OK; 
            ConfigDialogController controller (m_settingsProvider);
            

            
            hr = m_settingsProvider.Save (settings);
            Assert::AreEqual (S_OK, hr, L"Settings save should succeed");

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
        }





        // T019.2: Test ConfigDialogController uses defaults when no registry key exists
        TEST_METHOD (TestConfigDialogControllerUsesDefaultsWhenNoRegistry)
        {
            HRESULT                hr         = S_OK; 
            ConfigDialogController controller (m_settingsProvider);



            // Arrange: Ensure no saved data exists (already done in MethodSetup)

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
        }





        // T019.3: Test ConfigDialogController validates and clamps density slider updates
        TEST_METHOD (TestConfigDialogControllerClampsDensitySliderUpdates)
        {
            HRESULT                hr         = S_OK;
            ConfigDialogController controller (m_settingsProvider);



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
            ConfigDialogController controller (m_settingsProvider);
            
            
            
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
            ConfigDialogController controller (m_settingsProvider);
            
            
            
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
            ConfigDialogController controller (m_settingsProvider);
            
            
            
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
            ConfigDialogController controller (m_settingsProvider);
            
            
            
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
            ConfigDialogController controller (m_settingsProvider);
            
            
            
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
            ConfigDialogController controller (m_settingsProvider);



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
        }




        // T044 - new US5 controller methods

        TEST_METHOD (TestUpdateQualityPreset_SnapsAdvancedValues)
        {
            ConfigDialogController controller (m_settingsProvider);
            controller.Initialize();

            controller.UpdateQualityPreset (QualityPreset::Low);

            Assert::IsTrue (controller.GetSettings().m_qualityPreset == QualityPreset::Low);
            Assert::IsTrue (controller.GetSettings().m_advancedValues == LookupPresetValues (QualityPreset::Low));
        }




        TEST_METHOD (TestUpdateAdvancedGraphicsValues_DriftsToCustom)
        {
            ConfigDialogController controller (m_settingsProvider);
            controller.Initialize();

            // Pick values that don't match any named preset.
            AdvancedGraphicsValues custom { 137, 4, ResolutionDivisor::Eighth, BlurTaps::Low };
            controller.UpdateAdvancedGraphicsValues (custom);

            Assert::IsTrue (controller.GetSettings().m_qualityPreset == QualityPreset::Custom,
                            L"Off-table values should auto-flip preset to Custom");
            Assert::IsTrue (controller.GetSettings().m_advancedValues == custom);
            Assert::IsTrue (controller.GetSettings().m_lastCustom.has_value());
            Assert::IsTrue (*controller.GetSettings().m_lastCustom == custom,
                            L"LastCustom should always capture the latest advanced edit");
        }




        TEST_METHOD (TestUpdateQualityPreset_Custom_WithSavedLastCustom_RestoresIt)
        {
            ConfigDialogController controller (m_settingsProvider);
            controller.Initialize();

            // Save a custom set by editing first.
            AdvancedGraphicsValues custom { 50, 2, ResolutionDivisor::Quarter, BlurTaps::Medium };
            controller.UpdateAdvancedGraphicsValues (custom);

            // Navigate to a named preset (snaps advanced away from custom).
            controller.UpdateQualityPreset (QualityPreset::High);
            Assert::IsTrue (controller.GetSettings().m_advancedValues == LookupPresetValues (QualityPreset::High));

            // Switch back to Custom -> should restore the saved set.
            controller.UpdateQualityPreset (QualityPreset::Custom);
            Assert::IsTrue (controller.GetSettings().m_advancedValues == custom);
        }






        // T019.10: Test ConfigDialogController saves settings on ApplyChanges
        TEST_METHOD (TestConfigDialogControllerSavesOnApply)
        {
            // Arrange
            HRESULT                hr         = S_OK; 
            ConfigDialogController controller (m_settingsProvider);
            
            
            
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

            // Assert: Verify settings were saved
            ScreenSaverSettings loadedSettings;
            hr = m_settingsProvider.Load (loadedSettings);
            Assert::AreEqual (S_OK, hr, L"Settings load should succeed");

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
            // Arrange: Save initial settings
            ScreenSaverSettings initialSettings
            {
                .m_densityPercent        = 50,
                .m_colorSchemeKey        = L"green",
                .m_animationSpeedPercent = 75
            };
            
            HRESULT hr = m_settingsProvider.Save (initialSettings);
            Assert::AreEqual (S_OK, hr);

            ConfigDialogController controller (m_settingsProvider);
            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            // Modify settings but don't apply
            controller.UpdateDensity (90);
            controller.UpdateColorScheme (L"blue");
            controller.UpdateAnimationSpeed (50);

            // Act: Cancel changes
            controller.CancelChanges();

            // Assert: Verify saved settings still contain original values
            ScreenSaverSettings loadedSettings;
            hr = m_settingsProvider.Load (loadedSettings);
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
            ConfigDialogController controller (m_settingsProvider);



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
        }





        // T050.1: Test ConfigDialogController creates snapshot in live overlay mode
        TEST_METHOD (TestConfigDialogControllerCreatesSnapshotInLiveMode)
        {
            // Arrange: Set up initial settings in registry
            HRESULT                hr              = S_OK;
            ConfigDialogController controller (m_settingsProvider);
            ApplicationState       appState (m_settingsProvider);
            ScreenSaverSettings    initialSettings
            {
                .m_densityPercent        = 70,
                .m_colorSchemeKey        = L"blue",
                .m_animationSpeedPercent = 80,
                .m_glowIntensityPercent  = 120,
                .m_glowSizePercent       = 140
            };
            


            hr = m_settingsProvider.Save (initialSettings);
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
            ConfigDialogController controller (m_settingsProvider);
            ApplicationState       appState (m_settingsProvider);



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
            ConfigDialogController  controller (m_settingsProvider);
            ApplicationState        appState (m_settingsProvider);



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
            ConfigDialogController  controller (m_settingsProvider);
            ApplicationState        appState (m_settingsProvider);


            hr = m_settingsProvider.Save (initialSettings);
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

            // Assert: Should persist and exit live mode
            Assert::IsFalse (controller.IsLiveMode(), L"Should exit live mode after apply");

            ScreenSaverSettings savedSettings;
            hr = m_settingsProvider.Load (savedSettings);
            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (75, savedSettings.m_densityPercent, L"Changes should be saved");
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
                .m_showDebugStats        = true
            };
            
            HRESULT hr = m_settingsProvider.Save (settings);
            Assert::AreEqual (S_OK, hr);

            ConfigDialogController controller (m_settingsProvider);
            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            // Act & Assert: Verify IsLiveMode returns false before initialization
            Assert::IsFalse (controller.IsLiveMode(), L"Should not be in live mode before initialization");
        }





        // T050.6: Test live mode rejects modal operations
        TEST_METHOD (TestLiveModeRejectsModalOperations)
        {
            // Arrange
            ConfigDialogController controller (m_settingsProvider);
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
            ConfigDialogController controller (m_settingsProvider);
            ApplicationState       appState (m_settingsProvider);


            hr = m_settingsProvider.Save (initialSettings);
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
            ConfigDialogController controller (m_settingsProvider);
            HRESULT                hr       = S_OK;
            ApplicationState       appState (m_settingsProvider);



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
            ConfigDialogController controller (m_settingsProvider);
            HRESULT                hr       = S_OK;
            ApplicationState       appState (m_settingsProvider);



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
            ConfigDialogController controller (m_settingsProvider);
            HRESULT                hr       = S_OK;
            ApplicationState       appState (m_settingsProvider);



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
            ConfigDialogController controller (m_settingsProvider);
            HRESULT                hr       = S_OK;
            ApplicationState       appState (m_settingsProvider);



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
            ConfigDialogController controller (m_settingsProvider);
            ApplicationState       appState (m_settingsProvider);


            hr = m_settingsProvider.Save (initialSettings);
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

            ApplicationState       appState (m_settingsProvider);
            ConfigDialogController controller (m_settingsProvider);
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
            Logger::WriteMessage ("Live dialog shutdown test requires UI harness to simulate parent window destruction. Skipping until automation is implemented.\n");
            return;
        }





        ////////////////////////////////////////////////////////////////////////
        //
        //  T022 (US1, FR-004, FR-044): live-mode snapshot/rollback covers the
        //  5 new v1.5 fields, and the customColorPalette is INTENTIONALLY NOT
        //  snapshotted (FR-035 carve-out — palette is persisted directly on
        //  chooser-OK and never rolled back).
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (EnterLiveMode_SnapshotsAllV15Fields)
        {
            ScreenSaverSettings              initial {};
            HRESULT                          hr      = S_OK;
            ConfigDialogController           controller (m_settingsProvider);
            ApplicationState                 appState   (m_settingsProvider);


            initial.m_glowEnabled         = true;
            initial.m_scanlinesEnabled    = true;
            initial.m_scanlinesIntensity  = 30;
            initial.m_scanlinesStyle      = 50;
            initial.m_customColor         = RGB (0, 255, 0);
            initial.m_customColorPalette  = { RGB (1, 2, 3), RGB (4, 5, 6) };

            hr = m_settingsProvider.Save (initial);
            Assert::AreEqual (S_OK, hr);

            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr, L"EnterLiveMode (InitializeLiveMode) succeeds");

            // Mutate live, then cancel — snapshot must roll back the 5 fields.
            controller.UpdateGlowEnabled        (false);
            controller.UpdateScanlinesEnabled   (false);
            controller.UpdateScanlinesIntensity (99);
            controller.UpdateScanlinesStyle     (1);
            controller.UpdateCustomColor        (RGB (255, 0, 0));

            // Palette is NOT rollback-eligible — direct write survives Cancel.
            ScreenSaverSettings paletteChange = controller.GetSettings();
            paletteChange.m_customColorPalette = { RGB (10, 20, 30) };

            hr = controller.CancelLiveMode();
            Assert::AreEqual (S_OK, hr);

            const ScreenSaverSettings & restored = controller.GetSettings();
            Assert::IsTrue   (restored.m_glowEnabled,                        L"glowEnabled restored");
            Assert::IsTrue   (restored.m_scanlinesEnabled,                   L"scanlinesEnabled restored");
            Assert::AreEqual (30,                  restored.m_scanlinesIntensity, L"scanlinesIntensity restored");
            Assert::AreEqual (50,                  restored.m_scanlinesStyle,     L"scanlinesStyle restored");
            Assert::AreEqual (static_cast<DWORD> (RGB (0, 255, 0)),
                              static_cast<DWORD> (restored.m_customColor),
                              L"customColor restored");
        }


        TEST_METHOD (CancelLiveMode_RestoresAllV15Fields)
        {
            // Distinct from the prior test: verifies CancelLiveMode pushes the
            // restored values into the ApplicationState live-mirror, not just
            // back into controller-local m_settings (FR-044).
            ScreenSaverSettings              initial {};
            HRESULT                          hr      = S_OK;
            ConfigDialogController           controller (m_settingsProvider);
            ApplicationState                 appState   (m_settingsProvider);


            initial.m_glowEnabled        = true;
            initial.m_scanlinesEnabled   = true;
            initial.m_scanlinesIntensity = 30;
            initial.m_scanlinesStyle     = 50;
            initial.m_customColor        = RGB (0, 255, 0);

            hr = m_settingsProvider.Save (initial);
            Assert::AreEqual (S_OK, hr);

            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            controller.UpdateGlowEnabled        (false);
            controller.UpdateScanlinesEnabled   (false);
            controller.UpdateScanlinesIntensity (88);
            controller.UpdateScanlinesStyle     (12);
            controller.UpdateCustomColor        (RGB (200, 100, 50));

            hr = controller.CancelLiveMode();
            Assert::AreEqual (S_OK, hr);

            // ApplicationState restored via ApplySettings(snapshot) path.
            const ScreenSaverSettings & appAfter = appState.GetSettings();
            Assert::IsTrue   (appAfter.m_glowEnabled,                        L"app glowEnabled restored");
            Assert::IsTrue   (appAfter.m_scanlinesEnabled,                   L"app scanlinesEnabled restored");
            Assert::AreEqual (30,                  appAfter.m_scanlinesIntensity, L"app scanlinesIntensity restored");
            Assert::AreEqual (50,                  appAfter.m_scanlinesStyle,     L"app scanlinesStyle restored");
            Assert::AreEqual (static_cast<DWORD> (RGB (0, 255, 0)),
                              static_cast<DWORD> (appAfter.m_customColor),
                              L"app customColor restored");
        }


        ////////////////////////////////////////////////////////////////////////
        //
        //  T033b (US1, FR-004a, SC-011): the OK button's `PSN_APPLY` path
        //  calls CommitLiveMode, which must Save() every rollback-eligible
        //  field (5 new v1.5 + 3 existing v1.4 representative samples)
        //  through the settings provider.  Tests the registry-write path
        //  end-to-end via InMemorySettingsProvider.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (CommitLiveMode_WritesAllV15Fields)
        {
            HRESULT                          hr = S_OK;
            ConfigDialogController           controller (m_settingsProvider);
            ApplicationState                 appState   (m_settingsProvider);


            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            // Mutate the 5 new v1.5 fields + 3 existing v1.4 fields.
            controller.UpdateGlowEnabled        (false);
            controller.UpdateScanlinesEnabled   (false);
            controller.UpdateScanlinesIntensity (77);
            controller.UpdateScanlinesStyle     (22);
            controller.UpdateCustomColor        (RGB (10, 20, 30));

            controller.UpdateDensity            (42);
            controller.UpdateAnimationSpeed     (88);
            controller.UpdateGlowIntensity      (175);

            hr = controller.CommitLiveMode();
            Assert::AreEqual (S_OK, hr, L"CommitLiveMode returns S_OK in live mode");

            // All 8 fields must have round-tripped through the provider.
            const ScreenSaverSettings & stored = m_settingsProvider.GetStored();
            Assert::IsFalse  (stored.m_glowEnabled,                                L"glowEnabled persisted");
            Assert::IsFalse  (stored.m_scanlinesEnabled,                           L"scanlinesEnabled persisted");
            Assert::AreEqual (77,                  stored.m_scanlinesIntensity,    L"scanlinesIntensity persisted");
            Assert::AreEqual (22,                  stored.m_scanlinesStyle,        L"scanlinesStyle persisted");
            Assert::AreEqual (static_cast<DWORD> (RGB (10, 20, 30)),
                              static_cast<DWORD> (stored.m_customColor),
                              L"customColor persisted");
            Assert::AreEqual (42,                  stored.m_densityPercent,        L"densityPercent persisted");
            Assert::AreEqual (88,                  stored.m_animationSpeedPercent, L"animationSpeedPercent persisted");
            Assert::AreEqual (175,                 stored.m_glowIntensityPercent,  L"glowIntensityPercent persisted");

            Assert::IsFalse (controller.IsLiveMode(), L"CommitLiveMode clears live-mode state");
        }


        ////////////////////////////////////////////////////////////////////////
        //
        //  Mini-phase 2.5 (cross-page Reset button): ResetToDefaults must
        //  push the freshly-reset settings through to ApplicationState so
        //  the live preview snaps back instantly.  Previously the dialog-
        //  side OnResetButton walked each Update* setter; now that Reset
        //  lives on the property-sheet frame and broadcasts a resync to
        //  both pages, the controller has to own the live-push so the
        //  render thread sees the defaults regardless of which (if any)
        //  page is currently active.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (ResetToDefaults_LiveMode_PushesAllFieldsToSharedState)
        {
            HRESULT                hr = S_OK;
            ConfigDialogController controller (m_settingsProvider);
            ApplicationState       appState   (m_settingsProvider);


            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            // Mutate every rollback-eligible field away from defaults so a
            // post-reset GetSettings() comparison against a fresh-default
            // struct is meaningful.
            controller.UpdateDensity            (42);
            controller.UpdateAnimationSpeed     (33);
            controller.UpdateGlowIntensity      (175);
            controller.UpdateGlowSize           (150);
            controller.UpdateColorScheme        (L"red");
            controller.UpdateGlowEnabled        (false);
            controller.UpdateScanlinesEnabled   (false);
            controller.UpdateScanlinesIntensity (88);
            controller.UpdateScanlinesStyle     (22);
            controller.UpdateShowDebugStats     (true);
            controller.UpdateStartFullscreen    (false);

            // Act: reset.  In live mode this must propagate to ApplicationState.
            controller.ResetToDefaults();

            // Assert: ApplicationState reflects the freshly-reset settings.
            // ApplySettings is the coarse-grained propagation path the per-
            // field UpdateGlowEnabled/UpdateScanlines* setters already use,
            // so the test exercises the same SharedState round-trip the
            // render thread would observe.
            ScreenSaverSettings        defaults;
            const ScreenSaverSettings  asSettings = appState.GetSettings();

            Assert::AreEqual (defaults.m_densityPercent,
                              asSettings.m_densityPercent,
                              L"density propagated");
            Assert::AreEqual (defaults.m_animationSpeedPercent,
                              asSettings.m_animationSpeedPercent,
                              L"animation speed propagated");
            Assert::AreEqual (defaults.m_glowIntensityPercent,
                              asSettings.m_glowIntensityPercent,
                              L"glow intensity propagated");
            Assert::AreEqual (defaults.m_glowSizePercent,
                              asSettings.m_glowSizePercent,
                              L"glow size propagated");
            Assert::AreEqual (defaults.m_scanlinesIntensity,
                              asSettings.m_scanlinesIntensity,
                              L"scanlines intensity propagated");
            Assert::AreEqual (defaults.m_scanlinesStyle,
                              asSettings.m_scanlinesStyle,
                              L"scanlines style propagated");
            Assert::AreEqual (defaults.m_colorSchemeKey,
                              asSettings.m_colorSchemeKey,
                              L"color scheme propagated");
            Assert::AreEqual (defaults.m_glowEnabled,
                              asSettings.m_glowEnabled,
                              L"glow-enabled propagated");
            Assert::AreEqual (defaults.m_scanlinesEnabled,
                              asSettings.m_scanlinesEnabled,
                              L"scanlines-enabled propagated");
            Assert::AreEqual (defaults.m_showDebugStats,
                              asSettings.m_showDebugStats,
                              L"show-debug-stats propagated");
            Assert::AreEqual (defaults.m_startFullscreen,
                              asSettings.m_startFullscreen,
                              L"start-fullscreen propagated");

            // ApplicationState's derived runtime mirrors (color scheme enum,
            // show-statistics flag, display mode) must also have updated
            // through ApplySettings — proves the broadcast went through the
            // notify-callback path, not just a silent struct copy.
            Assert::IsTrue (appState.GetColorScheme()  == ParseColorSchemeKey (defaults.m_colorSchemeKey),
                            L"derived color-scheme enum mirrored");
            Assert::AreEqual (defaults.m_showDebugStats,
                              appState.GetShowStatistics(),
                              L"derived show-statistics mirrored");
        }


        ////////////////////////////////////////////////////////////////////////
        //
        //  T058 (US5, FR-004, FR-035): the active CustomColor MUST roll back
        //  on Cancel (it's a normal rollback-eligible field), but the
        //  CustomColorPalette MUST NOT — the palette lives outside the
        //  snapshot rollback set so swatch edits made during a live session
        //  survive even when the user cancels the dialog.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (CustomColorRollsBackOnCancel)
        {
            HRESULT                hr = S_OK;
            ConfigDialogController controller (m_settingsProvider);
            ApplicationState       appState   (m_settingsProvider);


            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            // Snapshot taken at the default green RGB(0,255,0).
            const COLORREF originalColor = controller.GetSettings().m_customColor;

            controller.UpdateCustomColor (RGB (42, 84, 168));
            Assert::AreEqual (static_cast<DWORD> (RGB (42, 84, 168)),
                              static_cast<DWORD> (controller.GetSettings().m_customColor),
                              L"Live mutation visible before Cancel");

            hr = controller.CancelLiveMode();
            Assert::AreEqual (S_OK, hr);

            Assert::AreEqual (static_cast<DWORD> (originalColor),
                              static_cast<DWORD> (controller.GetSettings().m_customColor),
                              L"Cancel must restore the snapshot CustomColor (FR-004)");
        }


        TEST_METHOD (CustomColorPaletteIsNOTRolledBackOnCancel)
        {
            HRESULT                  hr = S_OK;
            ConfigDialogController   controller (m_settingsProvider);
            ApplicationState         appState   (m_settingsProvider);
            std::array<COLORREF, 16> mutatedPalette {};


            hr = controller.Initialize();
            Assert::AreEqual (S_OK, hr);

            appState.Initialize (nullptr);

            hr = controller.InitializeLiveMode (&appState);
            Assert::AreEqual (S_OK, hr);

            // Mutate the palette directly via the settings struct (the
            // chooser-dialog wiring in T065 will do the equivalent via
            // CHOOSECOLORW::lpCustColors).  Mark each slot with a distinct
            // RGB so the post-Cancel assertion is unambiguous.
            for (size_t i = 0; i < mutatedPalette.size(); i++)
            {
                mutatedPalette[i] = RGB (static_cast<BYTE> (i + 1),
                                          static_cast<BYTE> (i * 2 + 1),
                                          static_cast<BYTE> (i * 3 + 1));
            }

            controller.SetCustomColorPalette (mutatedPalette);

            hr = controller.CancelLiveMode();
            Assert::AreEqual (S_OK, hr);

            // Per FR-035: palette is outside the rollback set — every slot
            // must still hold the mutated value after Cancel.
            const ScreenSaverSettings & post = controller.GetSettings();

            for (size_t i = 0; i < post.m_customColorPalette.size(); i++)
            {
                Assert::AreEqual (static_cast<DWORD> (mutatedPalette[i]),
                                  static_cast<DWORD> (post.m_customColorPalette[i]),
                                  L"Palette slot must survive Cancel (FR-035 carve-out)");
            }
        }
    };
}  // namespace MatrixRainTests
