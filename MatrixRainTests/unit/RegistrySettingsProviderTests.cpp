#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\RegistrySettingsProvider.h"
#include "..\..\MatrixRainCore\ScreenSaverSettings.h"




namespace MatrixRainTests
{
    TEST_CLASS (RegistrySettingsProviderTests)
    {
    private:
        static constexpr LPCWSTR TEST_REGISTRY_KEY_PATH = L"Software\\relmer\\MatrixRain_Test";

        RegistrySettingsProvider m_provider {TEST_REGISTRY_KEY_PATH};
        
        // Helper to delete test registry key for cleanup
        static void DeleteTestRegistryKey()
        {
            RegDeleteTreeW (HKEY_CURRENT_USER, TEST_REGISTRY_KEY_PATH);
        }
        
    public:
        TEST_CLASS_CLEANUP (Cleanup)
        {
            DeleteTestRegistryKey();
        }
        
        
        // T016: Test registry settings load/save operations
        
        TEST_METHOD (TestLoadSettings_ReturnsDefaults_WhenNoRegistryKeyExists)
        {
            // Arrange
            DeleteTestRegistryKey();
            ScreenSaverSettings settings;
            
            
            // Act
            HRESULT hr = m_provider.Load (settings);
            
            
            // Assert - Load should return S_FALSE when key doesn't exist
            Assert::AreEqual (S_FALSE, hr, L"Load should return S_FALSE when registry key doesn't exist");
            
            // Verify defaults are preserved
            Assert::AreEqual (50, settings.m_densityPercent,         L"Default density should be 50%");
            Assert::AreEqual (75,  settings.m_animationSpeedPercent, L"Default animation speed should be 75%");
            Assert::AreEqual (100, settings.m_glowIntensityPercent,  L"Default glow intensity should be 100%");
            Assert::AreEqual (100, settings.m_glowSizePercent,       L"Default glow size should be 100%");

            Assert::IsTrue  (settings.m_startFullscreen, L"Default should start fullscreen");
            Assert::IsFalse (settings.m_showDebugStats,  L"Debug stats should be off by default");
        }
        
        
        
        
        
        
        TEST_METHOD (TestSaveSettings_WritesToRegistry)
        {
            // Arrange
            DeleteTestRegistryKey();
            ScreenSaverSettings settings;
            
            settings.m_densityPercent        = 75;
            settings.m_colorSchemeKey        = L"green";
            settings.m_animationSpeedPercent = 50;
            settings.m_glowIntensityPercent  = 150;
            settings.m_glowSizePercent       = 120;
            settings.m_startFullscreen       = false;
            settings.m_showDebugStats        = true;
            
            
            // Act
            HRESULT hr = m_provider.Save (settings);
            
            
            // Assert
            Assert::AreEqual (S_OK, hr, L"Save should succeed");
            
            // Verify values were actually written to registry by reading them directly
            HKEY        hKey      = nullptr;
            LSTATUS     status    = RegOpenKeyExW (HKEY_CURRENT_USER, TEST_REGISTRY_KEY_PATH, 0, KEY_READ, &hKey);
            DWORD       dwValue   = 0;
            DWORD       dwSize    = sizeof (DWORD);
            WCHAR       szValue[256];
            
            
            Assert::AreEqual ((LONG)ERROR_SUCCESS, (LONG)status, L"Registry key should exist after save");
            
            status = RegQueryValueExW (hKey, L"Density", nullptr, nullptr, (LPBYTE)&dwValue, &dwSize);
            Assert::AreEqual ((LONG)ERROR_SUCCESS, (LONG)status, L"Density value should exist");
            Assert::AreEqual (75, (int)dwValue, L"Density should be 75");
            
            dwSize = sizeof (szValue);
            status = RegQueryValueExW (hKey, L"ColorScheme", nullptr, nullptr, (LPBYTE)szValue, &dwSize);
            Assert::AreEqual ((LONG)ERROR_SUCCESS, (LONG)status, L"ColorScheme value should exist");
            Assert::AreEqual (L"green", szValue, L"ColorScheme should be 'green'");
            
            dwSize = sizeof (DWORD);
            status = RegQueryValueExW (hKey, L"AnimationSpeed", nullptr, nullptr, (LPBYTE)&dwValue, &dwSize);
            Assert::AreEqual ((LONG)ERROR_SUCCESS, (LONG)status, L"AnimationSpeed value should exist");
            Assert::AreEqual (50, (int)dwValue, L"AnimationSpeed should be 50");
            
            RegCloseKey (hKey);
        }
        
        
        
        
        
        
        TEST_METHOD (TestLoadSettings_ReadsFromRegistry_WhenKeyExists)
        {
            // Arrange
            DeleteTestRegistryKey();
            
            // First save settings to registry
            ScreenSaverSettings saveSettings;
            
            saveSettings.m_densityPercent        = 60;
            saveSettings.m_colorSchemeKey        = L"blue";
            saveSettings.m_animationSpeedPercent = 80;
            saveSettings.m_glowIntensityPercent  = 125;
            saveSettings.m_glowSizePercent       = 90;
            saveSettings.m_startFullscreen       = false;
            saveSettings.m_showDebugStats        = true;
            
            HRESULT hr = m_provider.Save (saveSettings);
            Assert::AreEqual (S_OK, hr, L"Save should succeed");
            
            
            // Act - Load into new settings object
            ScreenSaverSettings loadSettings;
            
            hr = m_provider.Load (loadSettings);
            
            
            // Assert
            Assert::AreEqual (S_OK, hr, L"Load should succeed when key exists");
            Assert::AreEqual (60,      loadSettings.m_densityPercent,        L"Density should match saved value");
            Assert::AreEqual (L"blue", loadSettings.m_colorSchemeKey.c_str(), L"ColorScheme should match saved value");
            Assert::AreEqual (80,      loadSettings.m_animationSpeedPercent, L"AnimationSpeed should match saved value");
            Assert::AreEqual (125,     loadSettings.m_glowIntensityPercent,  L"GlowIntensity should match saved value");
            Assert::AreEqual (90,      loadSettings.m_glowSizePercent,       L"GlowSize should match saved value");
            Assert::IsFalse  (loadSettings.m_startFullscreen, L"StartFullscreen should match saved value");
            Assert::IsTrue   (loadSettings.m_showDebugStats,  L"ShowDebugStats should match saved value");
        }




        TEST_METHOD (TestLoadSettings_MultiMonitor_DefaultsToTrue_WhenAbsent)
        {
            DeleteTestRegistryKey();

            HKEY    hKey   = nullptr;
            LSTATUS status = RegCreateKeyExW (HKEY_CURRENT_USER, TEST_REGISTRY_KEY_PATH, 0, nullptr,
                                              REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
            Assert::AreEqual ((LONG)ERROR_SUCCESS, (LONG)status);

            DWORD density = 50;
            RegSetValueExW (hKey, L"Density", 0, REG_DWORD, (const BYTE *)&density, sizeof (DWORD));
            RegCloseKey (hKey);


            ScreenSaverSettings settings;
            HRESULT             hr = m_provider.Load (settings);


            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (settings.m_multiMonitorEnabled, L"Absent MultiMonitor value should leave default (true)");
        }




        TEST_METHOD (TestSaveLoadRoundTrip_MultiMonitor_PreservesFalse)
        {
            DeleteTestRegistryKey();

            ScreenSaverSettings saveSettings;
            saveSettings.m_multiMonitorEnabled = false;


            HRESULT hr = m_provider.Save (saveSettings);
            Assert::AreEqual (S_OK, hr);


            ScreenSaverSettings loadSettings;
            hr = m_provider.Load (loadSettings);


            Assert::AreEqual (S_OK, hr);
            Assert::IsFalse  (loadSettings.m_multiMonitorEnabled, L"MultiMonitor=false should round-trip");
        }




        TEST_METHOD (TestSaveLoadRoundTrip_MultiMonitor_PreservesTrue)
        {
            DeleteTestRegistryKey();

            ScreenSaverSettings saveSettings;
            saveSettings.m_multiMonitorEnabled = true;


            HRESULT hr = m_provider.Save (saveSettings);
            Assert::AreEqual (S_OK, hr);


            ScreenSaverSettings loadSettings;
            loadSettings.m_multiMonitorEnabled = false; // pre-set opposite to prove the load overwrites
            hr = m_provider.Load (loadSettings);


            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (loadSettings.m_multiMonitorEnabled, L"MultiMonitor=true should round-trip");
        }




        TEST_METHOD (TestSaveLoadRoundTrip_GpuAdapter_PreservesDescription)
        {
            DeleteTestRegistryKey();

            ScreenSaverSettings saveSettings;
            saveSettings.m_gpuAdapter = L"NVIDIA GeForce RTX 3050 Ti Laptop GPU";


            HRESULT hr = m_provider.Save (saveSettings);
            Assert::AreEqual (S_OK, hr);


            ScreenSaverSettings loadSettings;
            hr = m_provider.Load (loadSettings);


            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (std::wstring (L"NVIDIA GeForce RTX 3050 Ti Laptop GPU"), loadSettings.m_gpuAdapter);
        }




        TEST_METHOD (TestSaveLoadRoundTrip_GpuAdapter_EmptyDescription)
        {
            DeleteTestRegistryKey();

            ScreenSaverSettings saveSettings;
            saveSettings.m_gpuAdapter = L"";


            HRESULT hr = m_provider.Save (saveSettings);
            Assert::AreEqual (S_OK, hr);


            ScreenSaverSettings loadSettings;
            loadSettings.m_gpuAdapter = L"non-empty";
            hr = m_provider.Load (loadSettings);


            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (std::wstring (L""), loadSettings.m_gpuAdapter);
        }




        TEST_METHOD (TestSaveLoadRoundTrip_QualityPreset_PreservesNamedPreset)
        {
            DeleteTestRegistryKey();

            ScreenSaverSettings saveSettings;
            saveSettings.m_qualityPreset = QualityPreset::Low;


            HRESULT hr = m_provider.Save (saveSettings);
            Assert::AreEqual (S_OK, hr);


            ScreenSaverSettings loadSettings;
            hr = m_provider.Load (loadSettings);


            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (loadSettings.m_qualityPreset == QualityPreset::Low);

            // Per the load logic, the named preset's row is automatically
            // applied to m_advancedValues.
            Assert::IsTrue   (loadSettings.m_advancedValues == LookupPresetValues (QualityPreset::Low));
        }




        TEST_METHOD (TestSaveLoadRoundTrip_LastCustom_AllOrNothing)
        {
            DeleteTestRegistryKey();

            ScreenSaverSettings saveSettings;
            saveSettings.m_qualityPreset = QualityPreset::Custom;
            saveSettings.m_lastCustom    = AdvancedGraphicsValues { 137, 4, ResolutionDivisor::Eighth, BlurTaps::Low };
            saveSettings.m_advancedValues = *saveSettings.m_lastCustom;


            HRESULT hr = m_provider.Save (saveSettings);
            Assert::AreEqual (S_OK, hr);


            ScreenSaverSettings loadSettings;
            hr = m_provider.Load (loadSettings);


            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (loadSettings.m_qualityPreset == QualityPreset::Custom);
            Assert::IsTrue   (loadSettings.m_lastCustom.has_value());
            Assert::AreEqual (137, loadSettings.m_lastCustom->m_glowIntensityPercent);
            Assert::AreEqual (4,   loadSettings.m_lastCustom->m_blurPasses);
            Assert::IsTrue   (loadSettings.m_lastCustom->m_bloomResolutionDivisor == ResolutionDivisor::Eighth);
            Assert::IsTrue   (loadSettings.m_lastCustom->m_blurTaps               == BlurTaps::Low);
            // Custom + LastCustom present -> advanced values restored from LastCustom.
            Assert::IsTrue   (loadSettings.m_advancedValues == *loadSettings.m_lastCustom);
        }




        TEST_METHOD (TestLoadSettings_LastCustom_MissingOneValue_IgnoresAll)
        {
            DeleteTestRegistryKey();

            // Manually set 3 of the 4 LastCustom values; omit the 4th to
            // exercise the all-or-nothing read contract.
            HKEY    hKey   = nullptr;
            LSTATUS status = RegCreateKeyExW (HKEY_CURRENT_USER, TEST_REGISTRY_KEY_PATH, 0, nullptr,
                                              REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
            Assert::AreEqual ((LONG)ERROR_SUCCESS, (LONG)status);

            DWORD intensity = 137;
            DWORD passes    = 4;
            DWORD smoothness = 5;
            // VALUE_LASTCUSTOM_RESOLUTION intentionally not written.

            RegSetValueExW (hKey, L"LastCustom_GlowIntensity", 0, REG_DWORD, (const BYTE *)&intensity,  sizeof (DWORD));
            RegSetValueExW (hKey, L"LastCustom_Passes",        0, REG_DWORD, (const BYTE *)&passes,     sizeof (DWORD));
            RegSetValueExW (hKey, L"LastCustom_Smoothness",    0, REG_DWORD, (const BYTE *)&smoothness, sizeof (DWORD));
            RegCloseKey (hKey);


            ScreenSaverSettings settings;
            HRESULT             hr = m_provider.Load (settings);


            Assert::AreEqual (S_OK, hr);
            Assert::IsFalse  (settings.m_lastCustom.has_value(),
                              L"Missing any LastCustom_* value should yield nullopt (all-or-nothing read contract)");
        }
        
        
        
        
        
        
        TEST_METHOD (TestSaveSettings_ClampsInvalidValues)
        {
            // Arrange
            ScreenSaverSettings settings;
            
            settings.m_densityPercent        = 250;    // Out of range
            settings.m_animationSpeedPercent = 150;    // Out of range
            settings.m_glowIntensityPercent  = -50;    // Out of range
            
            
            // Act
            settings.Clamp();
            
            
            // Assert
            Assert::AreEqual (100, settings.m_densityPercent,        L"Density should be clamped to MAX_DENSITY_PERCENT");
            Assert::AreEqual (100, settings.m_animationSpeedPercent, L"Animation speed should be clamped to MAX");
            Assert::AreEqual (0,   settings.m_glowIntensityPercent,  L"Glow intensity should be clamped to MIN");
        }
        
        
        
        
        
        
        TEST_METHOD (TestLoadSettings_HandlesCorruptedRegistry)
        {
            // Arrange
            DeleteTestRegistryKey();
            
            // Write invalid data directly to registry
            HKEY    hKey   = nullptr;
            LSTATUS status = RegCreateKeyExW (HKEY_CURRENT_USER, TEST_REGISTRY_KEY_PATH, 0, nullptr,
                                               REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
            
            Assert::AreEqual ((LONG)ERROR_SUCCESS, (LONG)status, L"Test setup should create registry key");
            
            // Write invalid density value (out of range)
            DWORD dwInvalidDensity = 500;
            
            RegSetValueExW (hKey, L"Density", 0, REG_DWORD, (const BYTE *)&dwInvalidDensity, sizeof (DWORD));
            RegCloseKey (hKey);
            
            
            // Act
            ScreenSaverSettings settings;
            HRESULT             hr = m_provider.Load (settings);
            
            
            // Assert - Load should succeed but values should be clamped
            Assert::AreEqual (S_OK, hr, L"Load should succeed even with corrupted values");
            Assert::AreEqual (100, settings.m_densityPercent, L"Invalid density should be clamped to MAX_DENSITY_PERCENT");
        }
        
        
        
        
        
        
        TEST_METHOD (TestRegistryPath_UsesCorrectHive)
        {
            // Arrange
            DeleteTestRegistryKey();
            ScreenSaverSettings settings;
            
            
            // Act
            HRESULT hr = m_provider.Save (settings);
            
            
            // Assert - Verify key was created in HKEY_CURRENT_USER
            HKEY    hKey   = nullptr;
            LSTATUS status = RegOpenKeyExW (HKEY_CURRENT_USER, TEST_REGISTRY_KEY_PATH, 0, KEY_READ, &hKey);
            
            Assert::AreEqual (S_OK, hr, L"Save should succeed");
            Assert::AreEqual ((LONG)ERROR_SUCCESS, (LONG)status, L"Registry key should exist in HKEY_CURRENT_USER");
            
            if (hKey != nullptr)
            {
                RegCloseKey (hKey);
            }
        }




        // v1.5 T010 — legacy ShowFadeTimers REG_DWORD must be silently ignored.
        // The field was removed from ScreenSaverSettings; absence of any read
        // path in the provider satisfies the requirement.  We assert Load()
        // succeeds when the legacy value is present, with no behavioural
        // impact on the new schema.
        TEST_METHOD (LegacyShowFadeTimersIsSilentlyIgnored)
        {
            DeleteTestRegistryKey();


            // Arrange: create the key and stamp the legacy value.
            HKEY    hKey   = nullptr;
            LSTATUS status = RegCreateKeyExW (HKEY_CURRENT_USER, TEST_REGISTRY_KEY_PATH,
                                              0, nullptr, REG_OPTION_NON_VOLATILE,
                                              KEY_WRITE, nullptr, &hKey, nullptr);
            Assert::AreEqual ((LONG)ERROR_SUCCESS, (LONG)status, L"Test setup should create registry key");

            DWORD dwLegacy = 1;
            RegSetValueExW (hKey, L"ShowFadeTimers", 0, REG_DWORD,
                            (const BYTE *)&dwLegacy, sizeof (DWORD));
            RegCloseKey (hKey);


            // Act
            ScreenSaverSettings settings;
            HRESULT             hr = m_provider.Load (settings);


            // Assert: Load completes successfully; nothing to read into.
            Assert::AreEqual (S_OK, hr, L"Load should succeed in presence of legacy ShowFadeTimers value");
        }
    };
}

