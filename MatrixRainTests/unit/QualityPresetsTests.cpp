#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\QualityPresets.h"
#include "..\..\MatrixRainCore\ScreenSaverSettings.h"




namespace MatrixRainTests
{


    static AdapterInfo MakeAdapter (const std::wstring & description, unsigned int vramMb, bool isSoftware)
    {
        AdapterInfo a;
        a.m_description     = description;
        a.m_luid            = LUID { 0, 0 };
        a.m_dedicatedVramMb = vramMb;
        a.m_isSoftware      = isSoftware;
        a.m_isDefault       = false;
        return a;
    }




    TEST_CLASS (QualityPresetsTests)
    {
        public:

            //
            //  LookupPresetValues — table validation
            //

            TEST_METHOD (LookupPresetValues_Low)
            {
                AdvancedGraphicsValues v = LookupPresetValues (QualityPreset::Low);

                Assert::AreEqual (75, v.m_glowIntensityPercent);
                Assert::AreEqual (1,  v.m_blurPasses);
                Assert::IsTrue   (v.m_bloomResolutionDivisor == ResolutionDivisor::Quarter);
                Assert::IsTrue   (v.m_blurTaps               == BlurTaps::Low);
            }




            TEST_METHOD (LookupPresetValues_Medium)
            {
                AdvancedGraphicsValues v = LookupPresetValues (QualityPreset::Medium);

                Assert::AreEqual (100, v.m_glowIntensityPercent);
                Assert::AreEqual (2,   v.m_blurPasses);
                Assert::IsTrue   (v.m_bloomResolutionDivisor == ResolutionDivisor::Half);
                Assert::IsTrue   (v.m_blurTaps               == BlurTaps::Medium);
            }




            TEST_METHOD (LookupPresetValues_High_MatchesCurrentDefault)
            {
                // FR-022: High preset must be visually identical to today's
                // default rendering.  These values mirror the existing hardcoded
                // constants in RenderSystem before parametrization.
                AdvancedGraphicsValues v = LookupPresetValues (QualityPreset::High);

                Assert::AreEqual (100, v.m_glowIntensityPercent);
                Assert::AreEqual (3,   v.m_blurPasses);
                Assert::IsTrue   (v.m_bloomResolutionDivisor == ResolutionDivisor::Half);
                Assert::IsTrue   (v.m_blurTaps               == BlurTaps::High);
            }




            //
            //  DetectActivePreset
            //

            TEST_METHOD (DetectActivePreset_ExactPresetRow_ReturnsThatPreset)
            {
                Assert::IsTrue (DetectActivePreset (LookupPresetValues (QualityPreset::Low))    == QualityPreset::Low);
                Assert::IsTrue (DetectActivePreset (LookupPresetValues (QualityPreset::Medium)) == QualityPreset::Medium);
                Assert::IsTrue (DetectActivePreset (LookupPresetValues (QualityPreset::High))   == QualityPreset::High);
            }




            TEST_METHOD (DetectActivePreset_OffTableValues_ReturnsCustom)
            {
                AdvancedGraphicsValues v;
                v.m_glowIntensityPercent   = 137;     // Not on any preset row
                v.m_blurPasses             = 4;       // No preset uses 4 passes
                v.m_bloomResolutionDivisor = ResolutionDivisor::Eighth;  // Custom-only
                v.m_blurTaps               = BlurTaps::High;

                Assert::IsTrue (DetectActivePreset (v) == QualityPreset::Custom);
            }




            //
            //  ApplyPresetSnap
            //

            TEST_METHOD (ApplyPresetSnap_NamedPreset_ReturnsLookup)
            {
                AdvancedGraphicsValues current { 50, 4, ResolutionDivisor::Full, BlurTaps::Low };

                AdvancedGraphicsValues result = ApplyPresetSnap (QualityPreset::High, current, std::nullopt);

                Assert::IsTrue (result == LookupPresetValues (QualityPreset::High));
            }




            TEST_METHOD (ApplyPresetSnap_Custom_WithSavedLastCustom_RestoresIt)
            {
                AdvancedGraphicsValues current    { 100, 3, ResolutionDivisor::Half,    BlurTaps::High };
                AdvancedGraphicsValues lastCustom { 137, 2, ResolutionDivisor::Eighth,  BlurTaps::Low  };

                AdvancedGraphicsValues result = ApplyPresetSnap (QualityPreset::Custom, current, lastCustom);

                Assert::IsTrue (result == lastCustom);
            }




            TEST_METHOD (ApplyPresetSnap_Custom_NoSavedLastCustom_KeepsCurrent)
            {
                AdvancedGraphicsValues current { 137, 2, ResolutionDivisor::Eighth, BlurTaps::Low };

                AdvancedGraphicsValues result = ApplyPresetSnap (QualityPreset::Custom, current, std::nullopt);

                Assert::IsTrue (result == current);
            }




            //
            //  PickDefaultQualityPreset — first-run heuristic
            //

            TEST_METHOD (PickDefault_DiscreteAdapter_ReturnsHigh)
            {
                std::vector<AdapterInfo> adapters {
                    MakeAdapter (L"Intel UHD Graphics 620",  100,  false),     // integrated
                    MakeAdapter (L"NVIDIA GeForce GTX 1650", 4096, false)      // discrete
                };

                Assert::IsTrue (PickDefaultQualityPreset (adapters, 1920ull * 1080ull) == QualityPreset::High);
            }




            TEST_METHOD (PickDefault_IntegratedOnly_LightLoad_ReturnsMedium)
            {
                std::vector<AdapterInfo> adapters {
                    MakeAdapter (L"Intel UHD Graphics 620", 128, false)
                };

                Assert::IsTrue (PickDefaultQualityPreset (adapters, 1920ull * 1080ull) == QualityPreset::Medium);
            }




            TEST_METHOD (PickDefault_IntegratedOnly_HeavyLoad_ReturnsLow)
            {
                std::vector<AdapterInfo> adapters {
                    MakeAdapter (L"Intel UHD Graphics 620", 128, false)
                };

                // Two 4K monitors -> > kHeavyTotalPixelsThreshold (16M)
                uint64_t pixels = 2ull * 3840ull * 2160ull;

                Assert::IsTrue (PickDefaultQualityPreset (adapters, pixels) == QualityPreset::Low);
            }




            TEST_METHOD (PickDefault_SoftwareAdaptersIgnored_ReturnsMedium)
            {
                std::vector<AdapterInfo> adapters {
                    MakeAdapter (L"Microsoft Basic Render Driver", 1024, true),   // software (ignored)
                    MakeAdapter (L"Intel UHD Graphics 620",         128, false)   // integrated
                };

                Assert::IsTrue (PickDefaultQualityPreset (adapters, 1920ull * 1080ull) == QualityPreset::Medium);
            }




            TEST_METHOD (PickDefault_HeuristicConstants_Pinned)
            {
                // Protect against silent retunes of the heuristic.
                Assert::AreEqual (256u,             kDiscreteVramThresholdMb);
                Assert::AreEqual (16'000'000ull,    kHeavyTotalPixelsThreshold);
            }


            // T047 (US3, FR-026, FR-040, data-model.md §5): preset switching
            // mutates AdvancedGraphicsValues only; scanline settings are
            // strictly orthogonal and must NOT be touched by any preset
            // operation.  Structurally guaranteed (scanline fields live on
            // ScreenSaverSettings, not AdvancedGraphicsValues) but pinned
            // so a future refactor can't sneak them in.
            TEST_METHOD (QualityPresetDoesNotMutateScanlineSettings)
            {
                ScreenSaverSettings settings;
                settings.m_scanlinesEnabled   = true;
                settings.m_scanlinesIntensity = 30;
                settings.m_scanlinesStyle     = 50;


                for (QualityPreset preset : { QualityPreset::Low,
                                              QualityPreset::Medium,
                                              QualityPreset::High,
                                              QualityPreset::Custom })
                {
                    AdvancedGraphicsValues snapped =
                        ApplyPresetSnap (preset, settings.m_advancedValues, settings.m_lastCustom);

                    settings.m_advancedValues = snapped;

                    Assert::IsTrue   (settings.m_scanlinesEnabled,
                                      L"ApplyPresetSnap must not touch m_scanlinesEnabled");
                    Assert::AreEqual (30, settings.m_scanlinesIntensity,
                                      L"ApplyPresetSnap must not touch m_scanlinesIntensity");
                    Assert::AreEqual (50, settings.m_scanlinesStyle,
                                      L"ApplyPresetSnap must not touch m_scanlinesStyle");
                }
            }
    };


}
