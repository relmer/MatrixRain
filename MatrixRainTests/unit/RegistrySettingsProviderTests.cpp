#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\RegistrySettingsProvider.h"
#include "..\..\MatrixRainCore\ScreenSaverSettings.h"




namespace MatrixRainTests
{
    TEST_CLASS (RegistrySettingsProviderTests)
    {
    private:
        static constexpr LPCWSTR TEST_REGISTRY_KEY_PATH = L"Software\\relmer\\MatrixRain_Test";
        
        
        // Helper to delete test registry key for cleanup
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
        
        
        // T016: Test registry settings load/save operations
        
        TEST_METHOD (TestLoadSettings_ReturnsDefaults_WhenNoRegistryKeyExists)
        {
            // Arrange
            DeleteTestRegistryKey();
            ScreenSaverSettings settings;
            
            
            // Act
            HRESULT hr = RegistrySettingsProvider::Load (settings);
            
            
            // Assert - Load should return S_FALSE when key doesn't exist
            Assert::AreEqual (S_FALSE, hr, L"Load should return S_FALSE when registry key doesn't exist");
            
            // Verify defaults are preserved
            Assert::AreEqual (80, settings.m_densityPercent,         L"Default density should be 80%");
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
            settings.m_showFadeTimers        = true;
            
            
            // Act
            HRESULT hr = RegistrySettingsProvider::Save (settings);
            
            
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
            
            HRESULT hr = RegistrySettingsProvider::Save (saveSettings);
            Assert::AreEqual (S_OK, hr, L"Save should succeed");
            
            
            // Act - Load into new settings object
            ScreenSaverSettings loadSettings;
            
            hr = RegistrySettingsProvider::Load (loadSettings);
            
            
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
            HRESULT             hr = RegistrySettingsProvider::Load (settings);
            
            
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
            HRESULT hr = RegistrySettingsProvider::Save (settings);
            
            
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
    };
}

