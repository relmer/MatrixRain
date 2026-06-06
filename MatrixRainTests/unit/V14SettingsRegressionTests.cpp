#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\RegistrySettingsProvider.h"
#include "..\..\MatrixRainCore\ScreenSaverSettings.h"





namespace MatrixRainTests
{


    // T072a (Phase 8 polish, SC-007): end-to-end byte-identical round-trip
    // of every field that existed in v1.4.  This is the regression guard
    // that catches accidental migration breakage when v1.5 (or any later
    // version) extends RegistrySettingsProvider::{Load,Save} with new
    // fields -- a sloppy refactor that zeroes-out an existing field on
    // read, mis-clamps a write, or swaps the registry-value name will
    // surface immediately as a failure here.
    //
    // Coverage: every v1.4 field that round-trips through the registry
    // (excludes derived state like advancedValues that recomputes from
    // qualityPreset + lastCustom on read).
    TEST_CLASS (V14SettingsRegressionTests)
    {
    private:
        static constexpr LPCWSTR TEST_REGISTRY_KEY_PATH = L"Software\\relmer\\MatrixRain_V14Regression";

        RegistrySettingsProvider m_provider {TEST_REGISTRY_KEY_PATH};


        static void DeleteTestRegistryKey()
        {
            RegDeleteTreeW (HKEY_CURRENT_USER, TEST_REGISTRY_KEY_PATH);
        }


    public:
        TEST_METHOD_INITIALIZE (Setup)
        {
            DeleteTestRegistryKey();
        }


        TEST_CLASS_CLEANUP (Cleanup)
        {
            DeleteTestRegistryKey();
        }


        TEST_METHOD (V14Fields_SaveLoadRoundTrip_ByteIdentical)
        {
            // Arrange: a settings struct populated with values that
            // differ from the in-class defaults so a "did nothing" Load
            // can't accidentally pass.  These mirror a realistic v1.4
            // installation: medium density, blue scheme, slower speed,
            // boosted glow, multi-monitor on, Intel iGPU pinned,
            // Medium preset with Custom-drifted advanced values.
            ScreenSaverSettings original;


            original.m_densityPercent        = 65;
            original.m_colorSchemeKey        = L"blue";
            original.m_animationSpeedPercent = 55;
            original.m_glowIntensityPercent  = 175;
            original.m_glowSizePercent       = 130;
            original.m_startFullscreen       = false;
            original.m_showDebugStats        = true;
            original.m_multiMonitorEnabled   = false;
            original.m_gpuAdapter            = L"Intel(R) UHD Graphics 630";
            original.m_qualityPreset         = QualityPreset::Custom;

            // Custom-drift LastCustom: not at any named preset row.
            AdvancedGraphicsValues lastCustom;
            lastCustom.m_glowIntensityPercent = 175;  // matches m_glowIntensityPercent
            lastCustom.m_blurPasses           = 2;
            lastCustom.m_bloomResolutionDivisor = ResolutionDivisor::Quarter;
            lastCustom.m_blurTaps             = BlurTaps::Medium;

            original.m_lastCustom    = lastCustom;
            original.m_advancedValues = lastCustom;  // Custom preset uses LastCustom directly

            // Act: save, then load into a freshly-defaulted struct.
            Assert::AreEqual (S_OK, m_provider.Save (original),
                              L"Save must succeed against the clean test hive");

            ScreenSaverSettings reloaded;

            Assert::AreEqual (S_OK, m_provider.Load (reloaded),
                              L"Load must succeed against the just-written hive");

            // Assert: every v1.4-era field must round-trip exactly.
            Assert::AreEqual (original.m_densityPercent,
                              reloaded.m_densityPercent,
                              L"densityPercent");
            Assert::AreEqual (original.m_colorSchemeKey,
                              reloaded.m_colorSchemeKey,
                              L"colorSchemeKey");
            Assert::AreEqual (original.m_animationSpeedPercent,
                              reloaded.m_animationSpeedPercent,
                              L"animationSpeedPercent");
            Assert::AreEqual (original.m_glowIntensityPercent,
                              reloaded.m_glowIntensityPercent,
                              L"glowIntensityPercent");
            Assert::AreEqual (original.m_glowSizePercent,
                              reloaded.m_glowSizePercent,
                              L"glowSizePercent");
            Assert::AreEqual (original.m_startFullscreen,
                              reloaded.m_startFullscreen,
                              L"startFullscreen");
            Assert::AreEqual (original.m_showDebugStats,
                              reloaded.m_showDebugStats,
                              L"showDebugStats");
            Assert::AreEqual (original.m_multiMonitorEnabled,
                              reloaded.m_multiMonitorEnabled,
                              L"multiMonitorEnabled");
            Assert::AreEqual (original.m_gpuAdapter,
                              reloaded.m_gpuAdapter,
                              L"gpuAdapter");
            Assert::IsTrue   (original.m_qualityPreset == reloaded.m_qualityPreset,
                              L"qualityPreset");

            // LastCustom + AdvancedValues -- the all-or-nothing read
            // contract means a partial-state regression surfaces here too.
            Assert::IsTrue   (reloaded.m_lastCustom.has_value(),
                              L"lastCustom must survive round-trip");
            Assert::AreEqual (lastCustom.m_glowIntensityPercent,
                              reloaded.m_lastCustom->m_glowIntensityPercent,
                              L"lastCustom glowIntensityPercent");
            Assert::AreEqual (lastCustom.m_blurPasses,
                              reloaded.m_lastCustom->m_blurPasses,
                              L"lastCustom blurPasses");
            Assert::IsTrue   (lastCustom.m_bloomResolutionDivisor == reloaded.m_lastCustom->m_bloomResolutionDivisor,
                              L"lastCustom bloomResolutionDivisor");
            Assert::IsTrue   (lastCustom.m_blurTaps == reloaded.m_lastCustom->m_blurTaps,
                              L"lastCustom blurTaps");

            Assert::AreEqual (lastCustom.m_glowIntensityPercent,
                              reloaded.m_advancedValues.m_glowIntensityPercent,
                              L"advancedValues (Custom = LastCustom)");
            Assert::AreEqual (lastCustom.m_blurPasses,
                              reloaded.m_advancedValues.m_blurPasses,
                              L"advancedValues blurPasses");
        }


        TEST_METHOD (V15AdditionsDoNotCorruptV14Fields)
        {
            // Arrange: v1.4 fields populated (per the round-trip test);
            // ALSO populate every v1.5-added field with non-default
            // values.  A bug where v1.5 read/write inadvertently
            // overlapped a v1.4 registry value would manifest as
            // either v1.4 corruption or v1.5 corruption -- both
            // covered.
            ScreenSaverSettings original;


            original.m_densityPercent        = 42;
            original.m_colorSchemeKey        = L"red";
            original.m_animationSpeedPercent = 88;
            original.m_glowIntensityPercent  = 99;
            original.m_glowSizePercent       = 111;
            original.m_startFullscreen       = false;
            original.m_showDebugStats        = true;
            original.m_multiMonitorEnabled   = false;
            original.m_gpuAdapter            = L"NVIDIA RTX 4090";
            original.m_qualityPreset         = QualityPreset::Low;

            // v1.5 additions
            original.m_glowEnabled           = false;
            original.m_scanlinesEnabled      = false;
            original.m_scanlinesIntensity    = 17;
            original.m_scanlinesStyle        = 83;
            original.m_customColor           = RGB (255, 128, 64);

            for (size_t i = 0; i < original.m_customColorPalette.size(); i++)
            {
                original.m_customColorPalette[i] = RGB (static_cast<BYTE> (i * 16),
                                                        static_cast<BYTE> (255 - i * 16),
                                                        static_cast<BYTE> (i * 8));
            }

            Assert::AreEqual (S_OK, m_provider.Save (original));

            ScreenSaverSettings reloaded;

            Assert::AreEqual (S_OK, m_provider.Load (reloaded));

            // v1.4 fields -- byte-identical
            Assert::AreEqual (42,                 reloaded.m_densityPercent,        L"v1.4 density not corrupted by v1.5 writes");
            Assert::AreEqual (std::wstring (L"red"), reloaded.m_colorSchemeKey,     L"v1.4 colorSchemeKey not corrupted");
            Assert::AreEqual (88,                 reloaded.m_animationSpeedPercent, L"v1.4 animSpeed not corrupted");
            Assert::AreEqual (99,                 reloaded.m_glowIntensityPercent,  L"v1.4 glowIntensity not corrupted");
            Assert::AreEqual (111,                reloaded.m_glowSizePercent,       L"v1.4 glowSize not corrupted");
            Assert::IsFalse  (reloaded.m_startFullscreen,                           L"v1.4 startFullscreen not corrupted");
            Assert::IsTrue   (reloaded.m_showDebugStats,                            L"v1.4 showDebugStats not corrupted");
            Assert::IsFalse  (reloaded.m_multiMonitorEnabled,                      L"v1.4 multiMonitor not corrupted");
            Assert::AreEqual (std::wstring (L"NVIDIA RTX 4090"), reloaded.m_gpuAdapter, L"v1.4 gpuAdapter not corrupted");
            Assert::IsTrue   (QualityPreset::Low == reloaded.m_qualityPreset,       L"v1.4 qualityPreset not corrupted");

            // v1.5 fields -- byte-identical (the converse regression guard)
            Assert::IsFalse  (reloaded.m_glowEnabled,                              L"v1.5 glowEnabled persisted");
            Assert::IsFalse  (reloaded.m_scanlinesEnabled,                         L"v1.5 scanlinesEnabled persisted");
            Assert::AreEqual (17,                 reloaded.m_scanlinesIntensity,   L"v1.5 scanlinesIntensity persisted");
            Assert::AreEqual (83,                 reloaded.m_scanlinesStyle,       L"v1.5 scanlinesStyle persisted");
            Assert::AreEqual (static_cast<DWORD> (RGB (255, 128, 64)),
                              static_cast<DWORD> (reloaded.m_customColor),
                              L"v1.5 customColor persisted");

            for (size_t i = 0; i < reloaded.m_customColorPalette.size(); i++)
            {
                Assert::AreEqual (static_cast<DWORD> (original.m_customColorPalette[i]),
                                  static_cast<DWORD> (reloaded.m_customColorPalette[i]),
                                  L"v1.5 customColorPalette slot persisted");
            }
        }
    };


}
