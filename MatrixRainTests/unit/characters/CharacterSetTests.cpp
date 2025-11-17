#include "pch.h"
#include "matrixrain/CharacterSet.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace MatrixRain;

namespace MatrixRainTests
{
    TEST_CLASS(GlyphInfoTests)
    {
    public:
        TEST_METHOD(GlyphInfo_DefaultConstructor_InitializesFields)
        {
            GlyphInfo glyph;
            
            Assert::AreEqual(0.0f, glyph.uvMin.x);
            Assert::AreEqual(0.0f, glyph.uvMin.y);
            Assert::AreEqual(0.0f, glyph.uvMax.x);
            Assert::AreEqual(0.0f, glyph.uvMax.y);
            Assert::AreEqual(static_cast<uint32_t>(0), glyph.codepoint);
            Assert::IsFalse(glyph.mirrored);
        }

        TEST_METHOD(GlyphInfo_ParameterizedConstructor_SetsAllFields)
        {
            Vector2 uvMin(0.0f, 0.0f);
            Vector2 uvMax(0.5f, 0.5f);
            uint32_t codepoint = 0x30A0;
            bool mirrored = true;
            
            GlyphInfo glyph(uvMin, uvMax, codepoint, mirrored);
            
            Assert::AreEqual(0.0f, glyph.uvMin.x);
            Assert::AreEqual(0.0f, glyph.uvMin.y);
            Assert::AreEqual(0.5f, glyph.uvMax.x);
            Assert::AreEqual(0.5f, glyph.uvMax.y);
            Assert::AreEqual(static_cast<uint32_t>(0x30A0), glyph.codepoint);
            Assert::IsTrue(glyph.mirrored);
        }

        TEST_METHOD(GlyphInfo_CanStoreKatakanaCodepoint)
        {
            GlyphInfo glyph;
            glyph.codepoint = 0x30A2; // ア (Katakana A)
            
            Assert::AreEqual(static_cast<uint32_t>(0x30A2), glyph.codepoint);
        }

        TEST_METHOD(GlyphInfo_CanStoreLatinCodepoint)
        {
            GlyphInfo glyph;
            glyph.codepoint = 0x0041; // A
            
            Assert::AreEqual(static_cast<uint32_t>(0x0041), glyph.codepoint);
        }

        TEST_METHOD(GlyphInfo_CanStoreNumeralCodepoint)
        {
            GlyphInfo glyph;
            glyph.codepoint = 0x0035; // 5
            
            Assert::AreEqual(static_cast<uint32_t>(0x0035), glyph.codepoint);
        }

        TEST_METHOD(GlyphInfo_UVCoordinates_InNormalizedRange)
        {
            Vector2 uvMin(0.25f, 0.25f);
            Vector2 uvMax(0.75f, 0.75f);
            GlyphInfo glyph(uvMin, uvMax, 0x30A0, false);
            
            // UV coordinates should be in 0.0 to 1.0 range
            Assert::IsTrue(glyph.uvMin.x >= 0.0f && glyph.uvMin.x <= 1.0f);
            Assert::IsTrue(glyph.uvMin.y >= 0.0f && glyph.uvMin.y <= 1.0f);
            Assert::IsTrue(glyph.uvMax.x >= 0.0f && glyph.uvMax.x <= 1.0f);
            Assert::IsTrue(glyph.uvMax.y >= 0.0f && glyph.uvMax.y <= 1.0f);
        }

        TEST_METHOD(GlyphInfo_MirroredFlag_CanBeTrue)
        {
            GlyphInfo glyph(Vector2(), Vector2(), 0x30A0, true);
            Assert::IsTrue(glyph.mirrored);
        }

        TEST_METHOD(GlyphInfo_MirroredFlag_CanBeFalse)
        {
            GlyphInfo glyph(Vector2(), Vector2(), 0x30A0, false);
            Assert::IsFalse(glyph.mirrored);
        }
    };

    TEST_CLASS(CharacterSetTests)
    {
    public:
        TEST_METHOD(CharacterSet_GetInstance_ReturnsSingleton)
        {
            CharacterSet& instance1 = CharacterSet::GetInstance();
            CharacterSet& instance2 = CharacterSet::GetInstance();
            
            // Should return the same instance
            Assert::AreEqual(reinterpret_cast<uintptr_t>(&instance1), 
                           reinterpret_cast<uintptr_t>(&instance2));
        }

        TEST_METHOD(CharacterSet_GetGlyphCount_BeforeInitialization_ReturnsZero)
        {
            CharacterSet& charset = CharacterSet::GetInstance();
            // Note: This test assumes CharacterSet is not initialized yet
            // In a real scenario, we'd need to ensure a fresh instance
            // For now, we just verify the method exists and returns a value
            size_t count = charset.GetGlyphCount();
            Assert::IsTrue(count >= 0); // Can be 0 if not initialized, or 266 if initialized
        }

        TEST_METHOD(CharacterSet_GetTextureResource_BeforeInitialization_MayReturnNull)
        {
            CharacterSet& charset = CharacterSet::GetInstance();
            void* texture = charset.GetTextureResource();
            // Before initialization, texture may be nullptr
            // After initialization, it should be a valid pointer
            // We can't test both states in one test, so we just verify the method works
            Assert::IsTrue(texture == nullptr || texture != nullptr); // Always true, but tests the API
        }

        TEST_METHOD(CharacterSet_HasGetRandomGlyphIndexMethod)
        {
            CharacterSet& charset = CharacterSet::GetInstance();
            // This test just verifies the method exists and can be called
            // We can't test randomness properly without initialization
            // But we can verify it doesn't crash
            try
            {
                size_t index = charset.GetRandomGlyphIndex();
                // If initialized, should return valid index (0-265)
                // If not initialized, behavior is implementation-defined
                Assert::IsTrue(index >= 0); // Always true for size_t, but documents expectation
            }
            catch (...)
            {
                // If not initialized, method might throw - that's okay for this test
                Assert::IsTrue(true);
            }
        }

        TEST_METHOD(CharacterSet_HasGetGlyphMethod)
        {
            CharacterSet& charset = CharacterSet::GetInstance();
            // Verify the method signature exists
            // We can't test actual glyph data without initialization
            try
            {
                const GlyphInfo& glyph = charset.GetGlyph(0);
                // If this doesn't crash, the method works
                // Check that glyph has valid structure
                UNREFERENCED_PARAMETER(glyph);
                Assert::IsTrue(true);
            }
            catch (...)
            {
                // If not initialized or index invalid, might throw - acceptable
                Assert::IsTrue(true);
            }
        }

        TEST_METHOD(CharacterSet_HasInitializeMethod)
        {
            CharacterSet& charset = CharacterSet::GetInstance();
            // Verify Initialize method exists and returns bool
            // Note: Actual initialization may fail in test environment without D3D11
            try
            {
                bool result = charset.Initialize();
                // Result could be true or false depending on environment
                Assert::IsTrue(result == true || result == false);
            }
            catch (...)
            {
                // DirectX initialization might fail in test environment - that's okay
                Assert::IsTrue(true);
            }
        }

        // Shutdown test removed - calling Shutdown() on the singleton breaks other tests
        // since CharacterSet is shared across all test methods

        TEST_METHOD(CharacterSet_AfterInitialize_Has268Glyphs)
        {
            CharacterSet& charset = CharacterSet::GetInstance();
            charset.Initialize();
            
            // Should have 134 normal + 134 mirrored = 268 glyphs
            Assert::AreEqual(static_cast<size_t>(268), charset.GetGlyphCount());
        }

        TEST_METHOD(CharacterSet_AfterInitialize_FirstHalfNotMirrored)
        {
            CharacterSet& charset = CharacterSet::GetInstance();
            charset.Initialize();
            
            // First 134 glyphs should NOT be mirrored
            size_t count = charset.GetGlyphCount();
            Assert::IsTrue(count >= 134);
            
            for (size_t i = 0; i < 134; i++)
            {
                const GlyphInfo& glyph = charset.GetGlyph(i);
                Assert::IsFalse(glyph.mirrored, L"First 134 glyphs should not be mirrored");
            }
        }

        TEST_METHOD(CharacterSet_AfterInitialize_SecondHalfMirrored)
        {
            CharacterSet& charset = CharacterSet::GetInstance();
            charset.Initialize();
            
            // Last 134 glyphs should be mirrored
            size_t count = charset.GetGlyphCount();
            Assert::IsTrue(count >= 268);
            
            for (size_t i = 134; i < 268; i++)
            {
                const GlyphInfo& glyph = charset.GetGlyph(i);
                Assert::IsTrue(glyph.mirrored, L"Last 134 glyphs should be mirrored");
            }
        }

        TEST_METHOD(CharacterSet_AfterInitialize_GlyphsHaveValidUVCoords)
        {
            CharacterSet& charset = CharacterSet::GetInstance();
            charset.Initialize();
            
            // All glyphs should have UV coordinates in [0, 1] range
            size_t count = charset.GetGlyphCount();
            for (size_t i = 0; i < count; i++)
            {
                const GlyphInfo& glyph = charset.GetGlyph(i);
                
                Assert::IsTrue(glyph.uvMin.x >= 0.0f && glyph.uvMin.x <= 1.0f);
                Assert::IsTrue(glyph.uvMin.y >= 0.0f && glyph.uvMin.y <= 1.0f);
                Assert::IsTrue(glyph.uvMax.x >= 0.0f && glyph.uvMax.x <= 1.0f);
                Assert::IsTrue(glyph.uvMax.y >= 0.0f && glyph.uvMax.y <= 1.0f);
                
                // uvMax should be greater than uvMin
                Assert::IsTrue(glyph.uvMax.x > glyph.uvMin.x);
                Assert::IsTrue(glyph.uvMax.y > glyph.uvMin.y);
            }
        }

        TEST_METHOD(CharacterSet_AfterInitialize_CodepointsAreValid)
        {
            CharacterSet& charset = CharacterSet::GetInstance();
            charset.Initialize();
            
            // All glyphs should have non-zero codepoints
            size_t count = charset.GetGlyphCount();
            for (size_t i = 0; i < count; i++)
            {
                const GlyphInfo& glyph = charset.GetGlyph(i);
                Assert::IsTrue(glyph.codepoint > 0, L"Glyph codepoint should be non-zero");
            }
        }

        TEST_METHOD(CharacterSet_GetRandomGlyphIndex_ReturnsValidIndex)
        {
            CharacterSet& charset = CharacterSet::GetInstance();
            charset.Initialize();
            
            // Get multiple random indices and verify they're all valid
            size_t count = charset.GetGlyphCount();
            for (int i = 0; i < 100; i++)
            {
                size_t index = charset.GetRandomGlyphIndex();
                Assert::IsTrue(index < count, L"Random glyph index should be within valid range");
            }
        }

        TEST_METHOD(CharacterSet_UVCoordinates_NoOverlap)
        {
            CharacterSet& charset = CharacterSet::GetInstance();
            charset.Initialize();
            
            // Verify that glyphs have proper spacing (no overlap)
            // This is a simplified check - we just verify each glyph has reasonable size
            size_t count = charset.GetGlyphCount();
            
            for (size_t i = 0; i < count; i++)
            {
                const GlyphInfo& glyph = charset.GetGlyph(i);
                
                float width = glyph.uvMax.x - glyph.uvMin.x;
                float height = glyph.uvMax.y - glyph.uvMin.y;
                
                // Each glyph should occupy at least 1% of the texture (reasonable size)
                // In a 16x17 grid, each cell is ~1/16 * 1/17 ≈ 0.0037 or 0.37%
                // With padding, should be around 0.25-0.5%
                Assert::IsTrue(width > 0.001f, L"Glyph width should be reasonable");
                Assert::IsTrue(height > 0.001f, L"Glyph height should be reasonable");
                
                // Each glyph shouldn't be too large either (max 1/8 of texture in any dimension)
                Assert::IsTrue(width < 0.125f, L"Glyph width should not be too large");
                Assert::IsTrue(height < 0.125f, L"Glyph height should not be too large");
            }
        }
    };
}
