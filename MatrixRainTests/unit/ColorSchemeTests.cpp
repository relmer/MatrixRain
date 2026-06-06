#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ColorScheme.h"





namespace MatrixRainTests
{
    TEST_CLASS (ColorSchemeTests)
    {
        public:
            // T175: Test ColorScheme enum values
            TEST_METHOD (TestColorSchemeEnumValues)
            {
                // Verify all four color schemes exist
                ColorScheme green = ColorScheme::Green;
                ColorScheme blue = ColorScheme::Blue;
                ColorScheme red = ColorScheme::Red;
                ColorScheme amber = ColorScheme::Amber;

                // Each should have distinct values
                Assert::AreNotEqual (static_cast<int>(green), static_cast<int>(blue));
                Assert::AreNotEqual (static_cast<int>(blue), static_cast<int>(red));
                Assert::AreNotEqual (static_cast<int>(red), static_cast<int>(amber));
            }





            // T176: Test ColorScheme cycling sequence
            TEST_METHOD (TestColorSchemeCyclingSequence)
            {
                ColorScheme current = ColorScheme::Green;

                // Green → Blue
                current = GetNextColorScheme(current);
                Assert::IsTrue (current == ColorScheme::Blue, L"Green should cycle to Blue");

                // Blue → Red
                current = GetNextColorScheme(current);
                Assert::IsTrue (current == ColorScheme::Red, L"Blue should cycle to Red");

                // Red → Amber
                current = GetNextColorScheme(current);
                Assert::IsTrue (current == ColorScheme::Amber, L"Red should cycle to Amber");

                // Amber → ColorCycle
                current = GetNextColorScheme(current);
                Assert::IsTrue (current == ColorScheme::ColorCycle, L"Amber should cycle to ColorCycle");

                // ColorCycle → Green (wraps around)
                current = GetNextColorScheme(current);
                Assert::IsTrue (current == ColorScheme::Green, L"ColorCycle should cycle back to Green");
            }





            // T178: Test color scheme RGB values
            TEST_METHOD (TestColorSchemeRGBValues)
            {
                Color4 green = GetColorRGB(ColorScheme::Green);
                Assert::AreEqual (0.0f, green.r, 0.01f, L"Green R component");
                Assert::AreEqual (1.0f, green.g, 0.01f, L"Green G component");
                Assert::AreEqual (0.39f, green.b, 0.01f, L"Green B component (100/255)");

                Color4 blue = GetColorRGB(ColorScheme::Blue);
                Assert::AreEqual (0.0f, blue.r, 0.01f, L"Blue R component");
                Assert::AreEqual (0.39f, blue.g, 0.01f, L"Blue G component (100/255)");
                Assert::AreEqual (1.0f, blue.b, 0.01f, L"Blue B component");

                Color4 red = GetColorRGB(ColorScheme::Red);
                Assert::AreEqual (1.0f, red.r, 0.01f, L"Red R component");
                Assert::AreEqual (0.20f, red.g, 0.01f, L"Red G component (50/255)");
                Assert::AreEqual (0.20f, red.b, 0.01f, L"Red B component (50/255)");

                Color4 amber = GetColorRGB(ColorScheme::Amber);
                Assert::AreEqual (1.0f, amber.r, 0.01f, L"Amber R component");
                Assert::AreEqual (0.75f, amber.g, 0.01f, L"Amber G component (191/255)");
                Assert::AreEqual (0.0f, amber.b, 0.01f, L"Amber B component");
            }

            // Test default color scheme is Green
            TEST_METHOD (TestDefaultColorSchemeIsGreen)
            {
                ColorScheme defaultScheme = ColorScheme::Green;
                Color4 color = GetColorRGB(defaultScheme);

                // Should be green (0, 255, 100)
                Assert::IsTrue (color.g > 0.9f, L"Default scheme should be predominantly green");
            }


            ////////////////////////////////////////////////////////////////
            //
            //  T056 (US5, FR-033, FR-039, SC-007): Custom color scheme.
            //  The Custom slot is appended at ordinal 5; v1.4 ordinals
            //  (Green=0, Blue=1, Red=2, Amber=3, ColorCycle=4) MUST
            //  remain frozen so existing v1.4 registry values round-trip
            //  unchanged on a v1.5 upgrade.
            //
            ////////////////////////////////////////////////////////////////

            TEST_METHOD (CustomColorSchemeIsOrdinal5)
            {
                Assert::AreEqual (5, static_cast<int> (ColorScheme::Custom),
                                  L"Custom MUST be ordinal 5 (appended past ColorCycle=4)");
            }


            TEST_METHOD (V14OrdinalsAreFrozen)
            {
                Assert::AreEqual (0, static_cast<int> (ColorScheme::Green),       L"Green still 0");
                Assert::AreEqual (1, static_cast<int> (ColorScheme::Blue),        L"Blue still 1");
                Assert::AreEqual (2, static_cast<int> (ColorScheme::Red),         L"Red still 2");
                Assert::AreEqual (3, static_cast<int> (ColorScheme::Amber),       L"Amber still 3");
                Assert::AreEqual (4, static_cast<int> (ColorScheme::ColorCycle),  L"ColorCycle still 4");
            }


            TEST_METHOD (CustomColorSchemeRoundTripsViaKey)
            {
                Assert::IsTrue  (ColorScheme::Custom == ParseColorSchemeKey (L"custom"),
                                 L"L\"custom\" parses to ColorScheme::Custom");
                Assert::AreEqual (std::wstring (L"custom"), ColorSchemeToKey (ColorScheme::Custom),
                                  L"ColorScheme::Custom serialises to L\"custom\"");
                Assert::IsTrue  (IsValidColorSchemeKey (L"custom"), L"\"custom\" is valid");
                Assert::IsTrue  (IsValidColorSchemeKey (L"Custom"), L"case-insensitive valid");
            }


            TEST_METHOD (CustomColorSchemeIsNotInHotkeyCycle)
            {
                // GetNextColorScheme advances Green/Blue/Red/Amber/Cycle
                // back to Green; Custom is opt-in via the combo only and
                // must NOT appear in the hotkey rotation (FR-039).
                ColorScheme current = ColorScheme::ColorCycle;


                current = GetNextColorScheme (current);
                Assert::IsTrue (current == ColorScheme::Green,
                                L"ColorCycle wraps to Green, skipping Custom");
            }
    };
}  // namespace MatrixRainTests

