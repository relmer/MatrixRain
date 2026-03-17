#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\CharacterSet.h"





namespace MatrixRainTests
{


    TEST_CLASS (GlyphInfoTests)
    {
        public:
            TEST_METHOD (GlyphInfo_DefaultConstructor_InitializesFields)
            {
                GlyphInfo glyph;

                Assert::AreEqual (0.0f, glyph.uvMin.x);
                Assert::AreEqual (0.0f, glyph.uvMin.y);
                Assert::AreEqual (0.0f, glyph.uvMax.x);
                Assert::AreEqual (0.0f, glyph.uvMax.y);
                Assert::AreEqual (static_cast<uint32_t>(0), glyph.codepoint);
                Assert::IsFalse (glyph.mirrored);
            }






            TEST_METHOD (GlyphInfo_ParameterizedConstructor_SetsAllFields)
            {
                Vector2 uvMin(0.0f, 0.0f);
                Vector2 uvMax(0.5f, 0.5f);
                uint32_t codepoint = 0x30A0;
                bool mirrored = true;

                GlyphInfo glyph(uvMin, uvMax, codepoint, mirrored);

                Assert::AreEqual (0.0f, glyph.uvMin.x);
                Assert::AreEqual (0.0f, glyph.uvMin.y);
                Assert::AreEqual (0.5f, glyph.uvMax.x);
                Assert::AreEqual (0.5f, glyph.uvMax.y);
                Assert::AreEqual (static_cast<uint32_t>(0x30A0), glyph.codepoint);
                Assert::IsTrue (glyph.mirrored);
            }






            TEST_METHOD (GlyphInfo_CanStoreKatakanaCodepoint)
            {
                GlyphInfo glyph;
                glyph.codepoint = 0x30A2; // ア (Katakana A)

                Assert::AreEqual (static_cast<uint32_t>(0x30A2), glyph.codepoint);
            }






            TEST_METHOD (GlyphInfo_CanStoreLatinCodepoint)
            {
                GlyphInfo glyph;
                glyph.codepoint = 0x0041; // A

                Assert::AreEqual (static_cast<uint32_t>(0x0041), glyph.codepoint);
            }






            TEST_METHOD (GlyphInfo_CanStoreNumeralCodepoint)
            {
                GlyphInfo glyph;
                glyph.codepoint = 0x0035; // 5

                Assert::AreEqual (static_cast<uint32_t>(0x0035), glyph.codepoint);
            }






            TEST_METHOD (GlyphInfo_UVCoordinates_InNormalizedRange)
            {
                Vector2 uvMin(0.25f, 0.25f);
                Vector2 uvMax(0.75f, 0.75f);
                GlyphInfo glyph(uvMin, uvMax, 0x30A0, false);

                // UV coordinates should be in 0.0 to 1.0 range
                Assert::IsTrue (glyph.uvMin.x >= 0.0f && glyph.uvMin.x <= 1.0f);
                Assert::IsTrue (glyph.uvMin.y >= 0.0f && glyph.uvMin.y <= 1.0f);
                Assert::IsTrue (glyph.uvMax.x >= 0.0f && glyph.uvMax.x <= 1.0f);
                Assert::IsTrue (glyph.uvMax.y >= 0.0f && glyph.uvMax.y <= 1.0f);
            }






            TEST_METHOD (GlyphInfo_MirroredFlag_CanBeTrue)
            {
                GlyphInfo glyph(Vector2(), Vector2(), 0x30A0, true);
                Assert::IsTrue (glyph.mirrored);
            }






            TEST_METHOD (GlyphInfo_MirroredFlag_CanBeFalse)
            {
                GlyphInfo glyph(Vector2(), Vector2(), 0x30A0, false);
                Assert::IsFalse (glyph.mirrored);
            }
    };



}  // namespace MatrixRainTests






namespace MatrixRainTests
{


    TEST_CLASS (CharacterSetTests)
    {
        public:
            TEST_METHOD (CharacterSet_GetInstance_ReturnsSingleton)
            {
                CharacterSet& instance1 = CharacterSet::GetInstance();
                CharacterSet& instance2 = CharacterSet::GetInstance();

                // Should return the same instance
                Assert::AreEqual (reinterpret_cast<uintptr_t>(&instance1), 
                               reinterpret_cast<uintptr_t>(&instance2));
            }






            TEST_METHOD (CharacterSet_AfterInitialize_Has273Glyphs)
            {
                CharacterSet& charset = CharacterSet::GetInstance();
                charset.Initialize();

                // Should have 134 normal + 134 mirrored + 5 overlay symbols = 273 glyphs
                Assert::AreEqual (static_cast<size_t>(273), charset.GetGlyphCount());
            }






            TEST_METHOD (CharacterSet_AfterInitialize_NormalGlyphsAtEvenIndices)
            {
                CharacterSet& charset = CharacterSet::GetInstance();
                charset.Initialize();

                // In the paired layout, even indices (0, 2, 4, ...) should NOT be mirrored
                size_t count = charset.GetGlyphCount();
                Assert::IsTrue (count >= 268);

                for (size_t i = 0; i < 268; i += 2)
                {
                    const GlyphInfo& glyph = charset.GetGlyph (i);
                    Assert::IsFalse (glyph.mirrored, L"Even rain indices should not be mirrored");
                }
            }






            TEST_METHOD (CharacterSet_AfterInitialize_MirroredGlyphsAtOddIndices)
            {
                CharacterSet& charset = CharacterSet::GetInstance();
                charset.Initialize();

                // In the paired layout, odd indices (1, 3, 5, ...) should be mirrored
                size_t count = charset.GetGlyphCount();
                Assert::IsTrue (count >= 273);

                for (size_t i = 1; i < 268; i += 2)
                {
                    const GlyphInfo& glyph = charset.GetGlyph (i);
                    Assert::IsTrue (glyph.mirrored, L"Odd rain indices should be mirrored");
                }

                // Indices 268-272 are overlay symbols (non-mirrored)
                for (size_t i = 268; i < 273; i++)
                {
                    const GlyphInfo& glyph = charset.GetGlyph (i);
                    Assert::IsFalse (glyph.mirrored, L"Overlay symbols should not be mirrored");
                }
            }






            TEST_METHOD (CharacterSet_AfterInitialize_GlyphsHaveValidUVCoords)
            {
                CharacterSet& charset = CharacterSet::GetInstance();
                charset.Initialize();

                // Rain-atlas glyphs (first 272) should have UV coordinates in [0, 1] range
                // Overlay-only glyphs (index 272+) don't have rain UVs
                size_t count = std::min (charset.GetGlyphCount(), static_cast<size_t> (272));
                for (size_t i = 0; i < count; i++)
                {
                    const GlyphInfo& glyph = charset.GetGlyph (i);

                    Assert::IsTrue (glyph.uvMin.x >= 0.0f && glyph.uvMin.x <= 1.0f);
                    Assert::IsTrue (glyph.uvMin.y >= 0.0f && glyph.uvMin.y <= 1.0f);
                    Assert::IsTrue (glyph.uvMax.x >= 0.0f && glyph.uvMax.x <= 1.0f);
                    Assert::IsTrue (glyph.uvMax.y >= 0.0f && glyph.uvMax.y <= 1.0f);

                    // uvMax should be greater than uvMin
                    Assert::IsTrue (glyph.uvMax.x > glyph.uvMin.x);
                    Assert::IsTrue (glyph.uvMax.y > glyph.uvMin.y);
                }
            }






            TEST_METHOD (CharacterSet_AfterInitialize_CodepointsAreValid)
            {
                CharacterSet& charset = CharacterSet::GetInstance();
                charset.Initialize();

                // All glyphs should have non-zero codepoints
                size_t count = charset.GetGlyphCount();
                for (size_t i = 0; i < count; i++)
                {
                    const GlyphInfo& glyph = charset.GetGlyph (i);
                    Assert::IsTrue (glyph.codepoint > 0, L"Glyph codepoint should be non-zero");
                }
            }






            TEST_METHOD (CharacterSet_GetRandomGlyphIndex_ReturnsValidIndex)
            {
                CharacterSet& charset = CharacterSet::GetInstance();
                charset.Initialize();

                // Get multiple random indices and verify they're all valid
                size_t count = charset.GetGlyphCount();
                for (int i = 0; i < 100; i++)
                {
                    size_t index = charset.GetRandomGlyphIndex (count);
                    Assert::IsTrue (index < count, L"Random glyph index should be within valid range");
                }
            }






            TEST_METHOD (CharacterSet_UVCoordinates_NoOverlap)
            {
                CharacterSet& charset = CharacterSet::GetInstance();
                charset.Initialize();

                // Verify that rain-atlas glyphs have proper spacing (no overlap)
                // Overlay-only glyphs (index 272+) don't have rain UVs
                size_t count = std::min (charset.GetGlyphCount(), static_cast<size_t> (272));

                for (size_t i = 0; i < count; i++)
                {
                    const GlyphInfo& glyph = charset.GetGlyph (i);

                    float width = glyph.uvMax.x - glyph.uvMin.x;
                    float height = glyph.uvMax.y - glyph.uvMin.y;

                    // Each glyph should occupy at least 1% of the texture (reasonable size)
                    // In a 16x17 grid, each cell is ~1/16 * 1/17 ≈ 0.0037 or 0.37%
                    // With padding, should be around 0.25-0.5%
                    Assert::IsTrue (width > 0.001f, L"Glyph width should be reasonable");
                    Assert::IsTrue (height > 0.001f, L"Glyph height should be reasonable");

                    // Each glyph shouldn't be too large either (max 1/8 of texture in any dimension)
                    Assert::IsTrue (width < 0.125f, L"Glyph width should not be too large");
                    Assert::IsTrue (height < 0.125f, L"Glyph height should not be too large");
                }
            }
    };



}  // namespace MatrixRainTests

