#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ApplicationState.h"
#include "..\..\MatrixRainCore\ColorScheme.h"
#include "..\..\MatrixRainCore\ConfigDialogController.h"
#include "..\..\MatrixRainCore\InMemorySettingsProvider.h"





////////////////////////////////////////////////////////////////////////////////
//
//  The settings dialog's live preview, swept across every setting.
//
//  The per-feature tests checked the controller's copy of the settings and
//  ApplicationState's copy, and both were always right. Two bugs lived where
//  neither looked: Reset and Cancel never told the renderer about density,
//  speed or glow, and the preview saved those four to the registry, so a
//  canceled change came back on the next start. These tests check the two
//  things a user sees instead -- what the renderer is told, and what is
//  stored -- for every setting at once, through Cancel, Reset and OK.
//
//  Fully in memory: InMemorySettingsProvider stands in for the registry and
//  the renderer is a set of recording callbacks on ApplicationState.
//
//  The field lists below are explicit because C++ cannot enumerate a
//  struct's members. SettingsFieldMirror makes that safe: it repeats
//  ScreenSaverSettings' members, type for type and in order, so the two have
//  the same size in every configuration and on every architecture. Adding a
//  field to ScreenSaverSettings breaks the size check below, and this file
//  stops compiling until the new field is added here and to the sweep. (A
//  plain number does not work: std::wstring is larger in Debug builds.)
//
////////////////////////////////////////////////////////////////////////////////

struct SettingsFieldMirror
{
    int                                   densityPercent;
    std::wstring                          colorSchemeKey;
    int                                   animationSpeedPercent;
    int                                   glowIntensityPercent;
    int                                   glowSizePercent;
    bool                                  startFullscreen;
    bool                                  showDebugStats;
    bool                                  multiMonitorEnabled;
    std::wstring                          gpuAdapter;
    bool                                  glowEnabled;
    bool                                  scanlinesEnabled;
    int                                   scanlinesIntensity;
    int                                   scanlinesStyle;
    COLORREF                              customColor;
    HdrMode                               hdrMode;
    int                                   highlightBrightness;
    std::array<COLORREF, 16>              customColorPalette;
    QualityPreset                         qualityPreset;
    AdvancedGraphicsValues                advancedValues;
    std::optional<AdvancedGraphicsValues> lastCustom;
    std::optional<SystemClockTimePoint>   lastSavedTimestamp;
};

static_assert (sizeof (ScreenSaverSettings) == sizeof (SettingsFieldMirror),
               "ScreenSaverSettings changed: add the new field to SettingsFieldMirror and to the "
               "sweep in ConfigDialogLiveSweepTests.cpp (StoredDifferences, RendererDifferences, "
               "ApplyEverySetting)");





namespace MatrixRainTests
{


    ////////////////////////////////////////////////////////////////////////////
    //
    //  RendererView -- what ApplicationState has told the renderer, recorded
    //  from the same callbacks Application wires to SharedState.
    //
    ////////////////////////////////////////////////////////////////////////////

    struct RendererView
    {
        int                    density        { -1 };
        int                    animationSpeed { -1 };
        int                    glowIntensity  { -1 };
        int                    glowSize       { -1 };
        AdvancedGraphicsValues advanced       {};
        ColorScheme            colorScheme    { ColorScheme::Green };
        bool                   showStatistics { false };
        ScreenSaverSettings    live;          // The fields the v1.5 bulk callback carries
    };

    static void RecordRenderer (ApplicationState & appState, RendererView & view)
    {
        appState.RegisterDensityChangeCallback    ([&view] (int value)                          { view.density        = value; });
        appState.RegisterAnimationSpeedCallback   ([&view] (int value)                          { view.animationSpeed = value; });
        appState.RegisterGlowIntensityCallback    ([&view] (int value)                          { view.glowIntensity  = value; });
        appState.RegisterGlowSizeCallback         ([&view] (int value)                          { view.glowSize       = value; });
        appState.RegisterAdvancedGraphicsCallback ([&view] (const AdvancedGraphicsValues & v)   { view.advanced       = v;     });
        appState.RegisterColorSchemeCallback      ([&view] (ColorScheme value)                  { view.colorScheme    = value; });
        appState.RegisterShowStatisticsCallback   ([&view] (bool value)                         { view.showStatistics = value; });
        appState.RegisterV15LiveCallback          ([&view] (const ScreenSaverSettings & value)  { view.live           = value; });
    }





    ////////////////////////////////////////////////////////////////////////////
    //
    //  Field-by-field comparisons. Each returns the names of the fields that
    //  differ, so a failure says which setting went wrong.
    //
    ////////////////////////////////////////////////////////////////////////////

    static bool SameAdvanced (const AdvancedGraphicsValues & a, const AdvancedGraphicsValues & b)
    {
        return a.m_glowIntensityPercent   == b.m_glowIntensityPercent
            && a.m_blurPasses             == b.m_blurPasses
            && a.m_bloomResolutionDivisor == b.m_bloomResolutionDivisor
            && a.m_blurTaps               == b.m_blurTaps;
    }

    static bool SameOptionalAdvanced (const std::optional<AdvancedGraphicsValues> & a, const std::optional<AdvancedGraphicsValues> & b)
    {
        if (a.has_value() != b.has_value())
        {
            return false;
        }

        return !a.has_value() || SameAdvanced (*a, *b);
    }

    //  Every persisted field except two: the custom-color palette, which is
    //  deliberately outside Cancel and Reset (FR-035) and has its own tests,
    //  and the last-saved timestamp, which is bookkeeping.
    static std::wstring StoredDifferences (const ScreenSaverSettings & a, const ScreenSaverSettings & b)
    {
        std::wstring differences;



        auto check = [&differences] (bool same, const wchar_t * field)
        {
            if (!same)
            {
                differences += field;
                differences += L" ";
            }
        };

        check (a.m_densityPercent        == b.m_densityPercent,        L"density");
        check (a.m_colorSchemeKey        == b.m_colorSchemeKey,        L"colorSchemeKey");
        check (a.m_animationSpeedPercent == b.m_animationSpeedPercent, L"animationSpeed");
        check (a.m_glowIntensityPercent  == b.m_glowIntensityPercent,  L"glowIntensity");
        check (a.m_glowSizePercent       == b.m_glowSizePercent,       L"glowSize");
        check (a.m_startFullscreen       == b.m_startFullscreen,       L"startFullscreen");
        check (a.m_showDebugStats        == b.m_showDebugStats,        L"showDebugStats");
        check (a.m_multiMonitorEnabled   == b.m_multiMonitorEnabled,   L"multiMonitorEnabled");
        check (a.m_gpuAdapter            == b.m_gpuAdapter,            L"gpuAdapter");
        check (a.m_glowEnabled           == b.m_glowEnabled,           L"glowEnabled");
        check (a.m_scanlinesEnabled      == b.m_scanlinesEnabled,      L"scanlinesEnabled");
        check (a.m_scanlinesIntensity    == b.m_scanlinesIntensity,    L"scanlinesIntensity");
        check (a.m_scanlinesStyle        == b.m_scanlinesStyle,        L"scanlinesStyle");
        check (a.m_customColor           == b.m_customColor,           L"customColor");
        check (a.m_hdrMode               == b.m_hdrMode,               L"hdrMode");
        check (a.m_highlightBrightness   == b.m_highlightBrightness,   L"highlightBrightness");
        check (a.m_qualityPreset         == b.m_qualityPreset,         L"qualityPreset");
        check (SameAdvanced         (a.m_advancedValues, b.m_advancedValues), L"advancedValues");
        check (SameOptionalAdvanced (a.m_lastCustom,     b.m_lastCustom),     L"lastCustom");

        return differences;
    }

    //  Every field the running rain follows. The rest (start fullscreen,
    //  multi-monitor, GPU, preset name, last custom) take effect on restart
    //  or only through the values listed here.
    static std::wstring RendererDifferences (const RendererView & view, const ScreenSaverSettings & expected)
    {
        std::wstring differences;



        auto check = [&differences] (bool same, const wchar_t * field)
        {
            if (!same)
            {
                differences += field;
                differences += L" ";
            }
        };

        check (view.density        == expected.m_densityPercent,                       L"density");
        check (view.animationSpeed == expected.m_animationSpeedPercent,                L"animationSpeed");
        check (view.glowIntensity  == expected.m_glowIntensityPercent,                 L"glowIntensity");
        check (view.glowSize       == expected.m_glowSizePercent,                      L"glowSize");
        check (SameAdvanced (view.advanced, expected.m_advancedValues),                L"advancedValues");
        check (view.colorScheme    == ParseColorSchemeKey (expected.m_colorSchemeKey), L"colorScheme");
        check (view.showStatistics == expected.m_showDebugStats,                       L"showStatistics");
        check (view.live.m_glowEnabled         == expected.m_glowEnabled,              L"glowEnabled");
        check (view.live.m_scanlinesEnabled    == expected.m_scanlinesEnabled,         L"scanlinesEnabled");
        check (view.live.m_scanlinesIntensity  == expected.m_scanlinesIntensity,       L"scanlinesIntensity");
        check (view.live.m_scanlinesStyle      == expected.m_scanlinesStyle,           L"scanlinesStyle");
        check (view.live.m_customColor         == expected.m_customColor,              L"customColor");
        check (view.live.m_hdrMode             == expected.m_hdrMode,                  L"hdrMode");
        check (view.live.m_highlightBrightness == expected.m_highlightBrightness,      L"highlightBrightness");

        return differences;
    }





    ////////////////////////////////////////////////////////////////////////////
    //
    //  ApplyEverySetting -- moves every control in the dialog to a value that
    //  is not its default, through the controller's own setters, the way the
    //  dialog does.
    //
    ////////////////////////////////////////////////////////////////////////////

    static void ApplyEverySetting (ConfigDialogController & controller)
    {
        AdvancedGraphicsValues custom;



        custom.m_glowIntensityPercent   = 170;
        custom.m_blurPasses             = 1;
        custom.m_bloomResolutionDivisor = ResolutionDivisor::Quarter;
        custom.m_blurTaps               = BlurTaps::Low;

        controller.UpdateDensity                (23);
        controller.UpdateColorScheme            (L"blue");
        controller.UpdateAnimationSpeed         (33);
        controller.UpdateGlowIntensity          (170);
        controller.UpdateGlowSize               (60);
        controller.UpdateStartFullscreen        (false);
        controller.UpdateShowDebugStats         (true);
        controller.UpdateMultiMonitorEnabled    (false);
        controller.UpdateGpuAdapter             (L"Sweep Test GPU");
        controller.UpdateAdvancedGraphicsValues (custom);
        controller.UpdateGlowEnabled            (false);
        controller.UpdateScanlinesEnabled       (false);
        controller.UpdateScanlinesIntensity     (77);
        controller.UpdateScanlinesStyle         (11);
        controller.UpdateCustomColor            (RGB (1, 2, 3));
        controller.UpdateHdrMode                (HdrMode::Off);
        controller.UpdateHighlightBrightness    (33);
    }





    TEST_CLASS (ConfigDialogLiveSweepTests)
    {
        private:
            InMemorySettingsProvider m_settingsProvider;

        public:
            TEST_METHOD_INITIALIZE (MethodSetup)
            {
                m_settingsProvider.Clear();
            }




            TEST_METHOD (Sweep_MovesEveryField)
            {
                // The sweep is only as good as its coverage: every field it
                // compares must actually differ from the default after it,
                // or a comparison could pass without testing anything.
                ConfigDialogController controller (m_settingsProvider);
                ApplicationState       appState   (m_settingsProvider);
                std::wstring           unchanged;



                Assert::AreEqual (S_OK, controller.Initialize());
                appState.Initialize (nullptr);
                Assert::AreEqual (S_OK, controller.InitializeLiveMode (&appState));

                ApplyEverySetting (controller);

                {
                    const ScreenSaverSettings & moved    = controller.GetSettings();
                    const ScreenSaverSettings   defaults;
                    std::wstring                different = StoredDifferences (moved, defaults);
                    const wchar_t *             fields[]  = { L"density", L"colorSchemeKey", L"animationSpeed", L"glowIntensity",
                                                              L"glowSize", L"startFullscreen", L"showDebugStats",
                                                              L"multiMonitorEnabled", L"gpuAdapter", L"glowEnabled",
                                                              L"scanlinesEnabled", L"scanlinesIntensity", L"scanlinesStyle",
                                                              L"customColor", L"hdrMode", L"highlightBrightness",
                                                              L"qualityPreset", L"advancedValues", L"lastCustom" };


                    for (const wchar_t * field : fields)
                    {
                        if (different.find (std::wstring (field) + L" ") == std::wstring::npos)
                        {
                            unchanged += field;
                            unchanged += L" ";
                        }
                    }
                }

                Assert::IsTrue (unchanged.empty(), (L"The sweep left these at their defaults: " + unchanged).c_str());
            }




            TEST_METHOD (Cancel_RestoresEveryField_InTheRendererAndTheStore)
            {
                ConfigDialogController controller (m_settingsProvider);
                ApplicationState       appState   (m_settingsProvider);
                RendererView           renderer;
                ScreenSaverSettings    original;



                original.m_densityPercent = 61;   // Something other than the defaults to return to
                Assert::AreEqual (S_OK, m_settingsProvider.Save (original));

                Assert::AreEqual (S_OK, controller.Initialize());
                appState.Initialize (nullptr);
                original = m_settingsProvider.GetStored();

                RecordRenderer (appState, renderer);
                Assert::AreEqual (S_OK, controller.InitializeLiveMode (&appState));

                ApplyEverySetting (controller);

                {
                    const std::wstring stored = StoredDifferences (m_settingsProvider.GetStored(), original);


                    Assert::IsTrue (stored.empty(), (L"The preview saved: " + stored).c_str());
                }

                Assert::AreEqual (S_OK, controller.CancelLiveMode());

                {
                    const std::wstring rendered = RendererDifferences (renderer, original);
                    const std::wstring stored   = StoredDifferences   (m_settingsProvider.GetStored(), original);


                    Assert::IsTrue (rendered.empty(), (L"After Cancel the renderer still has: " + rendered).c_str());
                    Assert::IsTrue (stored.empty(),   (L"After Cancel the store changed: "     + stored).c_str());
                }
            }




            TEST_METHOD (ResetThenOk_AppliesAndStoresEveryDefault)
            {
                ConfigDialogController controller (m_settingsProvider);
                ApplicationState       appState   (m_settingsProvider);
                RendererView           renderer;
                const ScreenSaverSettings defaults;



                Assert::AreEqual (S_OK, controller.Initialize());
                appState.Initialize (nullptr);
                RecordRenderer (appState, renderer);
                Assert::AreEqual (S_OK, controller.InitializeLiveMode (&appState));

                ApplyEverySetting (controller);
                controller.ResetToDefaults();

                {
                    const std::wstring rendered = RendererDifferences (renderer, defaults);


                    Assert::IsTrue (rendered.empty(), (L"After Reset the renderer still has: " + rendered).c_str());
                }

                Assert::AreEqual (S_OK, controller.CommitLiveMode());

                {
                    const std::wstring stored = StoredDifferences (m_settingsProvider.GetStored(), defaults);


                    Assert::IsTrue (stored.empty(), (L"After Reset and OK the store differs from the defaults in: " + stored).c_str());
                }
            }




            TEST_METHOD (Ok_AppliesAndStoresEveryChange)
            {
                ConfigDialogController controller (m_settingsProvider);
                ApplicationState       appState   (m_settingsProvider);
                RendererView           renderer;
                ScreenSaverSettings    moved;



                Assert::AreEqual (S_OK, controller.Initialize());
                appState.Initialize (nullptr);
                RecordRenderer (appState, renderer);
                Assert::AreEqual (S_OK, controller.InitializeLiveMode (&appState));

                ApplyEverySetting (controller);
                moved = controller.GetSettings();

                {
                    const std::wstring rendered = RendererDifferences (renderer, moved);


                    Assert::IsTrue (rendered.empty(), (L"The preview did not reach the renderer for: " + rendered).c_str());
                }

                Assert::AreEqual (S_OK, controller.CommitLiveMode());

                {
                    const std::wstring stored = StoredDifferences (m_settingsProvider.GetStored(), moved);


                    Assert::IsTrue (stored.empty(), (L"OK did not store: " + stored).c_str());
                }
            }
    };
}
