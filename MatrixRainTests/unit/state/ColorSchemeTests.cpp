#include "pch.h"
#include "CppUnitTest.h"
#include "matrixrain/ColorScheme.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace MatrixRain;

namespace MatrixRainTests
{
    TEST_CLASS(ColorSchemeTests)
    {
    public:
        // T175: Test ColorScheme enum values
        TEST_METHOD(TestColorSchemeEnumValues)
        {
            // Verify all four color schemes exist
            ColorScheme green = ColorScheme::Green;
            ColorScheme blue = ColorScheme::Blue;
            ColorScheme red = ColorScheme::Red;
            ColorScheme amber = ColorScheme::Amber;
            
            // Each should have distinct values
            Assert::AreNotEqual(static_cast<int>(green), static_cast<int>(blue));
            Assert::AreNotEqual(static_cast<int>(blue), static_cast<int>(red));
            Assert::AreNotEqual(static_cast<int>(red), static_cast<int>(amber));
        }

        // T176: Test ColorScheme cycling sequence
        TEST_METHOD(TestColorSchemeCyclingSequence)
        {
            ColorScheme current = ColorScheme::Green;
            
            // Green → Blue
            current = GetNextColorScheme(current);
            Assert::IsTrue(current == ColorScheme::Blue, L"Green should cycle to Blue");
            
            // Blue → Red
            current = GetNextColorScheme(current);
            Assert::IsTrue(current == ColorScheme::Red, L"Blue should cycle to Red");
            
            // Red → Amber
            current = GetNextColorScheme(current);
            Assert::IsTrue(current == ColorScheme::Amber, L"Red should cycle to Amber");
            
            // Amber → Green (wraps around)
            current = GetNextColorScheme(current);
            Assert::IsTrue(current == ColorScheme::Green, L"Amber should cycle back to Green");
        }

        // T178: Test color scheme RGB values
        TEST_METHOD(TestColorSchemeRGBValues)
        {
            Color4 green = GetColorRGB(ColorScheme::Green);
            Assert::AreEqual(0.0f, green.r, 0.01f, L"Green R component");
            Assert::AreEqual(1.0f, green.g, 0.01f, L"Green G component");
            Assert::AreEqual(0.39f, green.b, 0.01f, L"Green B component (100/255)");
            
            Color4 blue = GetColorRGB(ColorScheme::Blue);
            Assert::AreEqual(0.0f, blue.r, 0.01f, L"Blue R component");
            Assert::AreEqual(0.39f, blue.g, 0.01f, L"Blue G component (100/255)");
            Assert::AreEqual(1.0f, blue.b, 0.01f, L"Blue B component");
            
            Color4 red = GetColorRGB(ColorScheme::Red);
            Assert::AreEqual(1.0f, red.r, 0.01f, L"Red R component");
            Assert::AreEqual(0.20f, red.g, 0.01f, L"Red G component (50/255)");
            Assert::AreEqual(0.20f, red.b, 0.01f, L"Red B component (50/255)");
            
            Color4 amber = GetColorRGB(ColorScheme::Amber);
            Assert::AreEqual(1.0f, amber.r, 0.01f, L"Amber R component");
            Assert::AreEqual(0.75f, amber.g, 0.01f, L"Amber G component (191/255)");
            Assert::AreEqual(0.0f, amber.b, 0.01f, L"Amber B component");
        }

        // Test default color scheme is Green
        TEST_METHOD(TestDefaultColorSchemeIsGreen)
        {
            ColorScheme defaultScheme = ColorScheme::Green;
            Color4 color = GetColorRGB(defaultScheme);
            
            // Should be green (0, 255, 100)
            Assert::IsTrue(color.g > 0.9f, L"Default scheme should be predominantly green");
        }
    };
}
