#include "pch.h"


#include "MatrixRain/Viewport.h"

#include "MatrixRain/Math.h"


namespace MatrixRainTests
{


    TEST_CLASS (ViewportTests)
    {
        public:
            TEST_METHOD (Viewport_DefaultConstructor_InitializesFields)
            {
                Viewport viewport;

                // Should have default dimensions (will be updated on first resize)
                Assert::AreEqual (0.0f, viewport.GetWidth());
                Assert::AreEqual (0.0f, viewport.GetHeight());
            }






            TEST_METHOD (Viewport_Resize_UpdatesDimensions)
            {
                Viewport viewport;

                viewport.Resize (1920.0f, 1080.0f);

                Assert::AreEqual (1920.0f, viewport.GetWidth());
                Assert::AreEqual (1080.0f, viewport.GetHeight());
            }






            TEST_METHOD (Viewport_Resize_UpdatesProjectionMatrix)
            {
                Viewport viewport;

                viewport.Resize (1920.0f, 1080.0f);

                const Matrix4x4& projection = viewport.GetProjectionMatrix();

                // Verify projection matrix is not identity (has been updated)
                // An orthographic projection will have non-zero values in specific positions
                Assert::AreNotEqual (0.0f, projection.m[0][0]); // X scale
                Assert::AreNotEqual (0.0f, projection.m[1][1]); // Y scale
                Assert::AreNotEqual (0.0f, projection.m[2][2]); // Z scale
            }






            TEST_METHOD (Viewport_ProjectionMatrix_IsOrthographic)
            {
                Viewport viewport;
                viewport.Resize (1920.0f, 1080.0f);

                const Matrix4x4& projection = viewport.GetProjectionMatrix();

                // Orthographic projection should have w=1 (not perspective divide)
                // The last column determines if it's perspective (last row would have z component for perspective)
                // For orthographic: bottom-right corner is 1.0, and third row doesn't affect W
                Assert::AreEqual (0.0f, projection.m[3][2]); // No Z affecting W
                Assert::AreEqual (1.0f, projection.m[3][3]); // W = 1 always (orthographic, not perspective)

                // Also verify it's not a perspective projection (which would have non-zero m[2][3])
                Assert::AreEqual (0.0f, projection.m[2][3]);
            }






            TEST_METHOD (Viewport_ProjectionMatrix_CoversViewportBounds)
            {
                Viewport viewport;
                float width = 1920.0f;
                float height = 1080.0f;
                viewport.Resize (width, height);

                const Matrix4x4& projection = viewport.GetProjectionMatrix();

                // The projection should map screen coordinates to NDC
                // For orthographic: X should span [0, width], Y should span [0, height]
                // The scale factors should be related to viewport dimensions
                float expectedXScale = 2.0f / width;
                float expectedYScale = 2.0f / height;

                // Allow some tolerance for floating point comparison
                float tolerance = 0.0001f;
                Assert::IsTrue (abs(projection.m[0][0] - expectedXScale) < tolerance);
                Assert::IsTrue (abs(projection.m[1][1] - (-expectedYScale)) < tolerance); // Y inverted for screen space
            }






            TEST_METHOD (Viewport_MultipleResizes_UpdatesCorrectly)
            {
                Viewport viewport;

                // First resize
                viewport.Resize (800.0f, 600.0f);
                Assert::AreEqual (800.0f, viewport.GetWidth());
                Assert::AreEqual (600.0f, viewport.GetHeight());

                // Second resize
                viewport.Resize (1920.0f, 1080.0f);
                Assert::AreEqual (1920.0f, viewport.GetWidth());
                Assert::AreEqual (1080.0f, viewport.GetHeight());

                // Third resize
                viewport.Resize (1280.0f, 720.0f);
                Assert::AreEqual (1280.0f, viewport.GetWidth());
                Assert::AreEqual (720.0f, viewport.GetHeight());
            }






            TEST_METHOD (Viewport_Resize_HandlesZeroDimensions)
            {
                Viewport viewport;

                // Should handle zero dimensions gracefully (might occur during minimize)
                viewport.Resize (0.0f, 0.0f);

                Assert::AreEqual (0.0f, viewport.GetWidth());
                Assert::AreEqual (0.0f, viewport.GetHeight());

                // Should be able to recover from zero dimensions
                viewport.Resize (1920.0f, 1080.0f);
                Assert::AreEqual (1920.0f, viewport.GetWidth());
                Assert::AreEqual (1080.0f, viewport.GetHeight());
            }






            TEST_METHOD (Viewport_GetAspectRatio_CalculatesCorrectly)
            {
                Viewport viewport;
                viewport.Resize (1920.0f, 1080.0f);

                float aspectRatio = viewport.GetAspectRatio();
                float expected = 1920.0f / 1080.0f;

                Assert::AreEqual (expected, aspectRatio, 0.0001f);
            }






            TEST_METHOD (Viewport_GetAspectRatio_HandlesZeroHeight)
            {
                Viewport viewport;
                viewport.Resize (1920.0f, 0.0f);

                float aspectRatio = viewport.GetAspectRatio();

                // Should return a safe default (e.g., 1.0) or handle gracefully
                Assert::IsTrue (aspectRatio >= 0.0f); // Just verify it doesn't crash or return negative
            }
    };



}  // namespace MatrixRainTests

