#include "pch.h"


#include "MatrixRain/Math.h"


namespace MatrixRainTests
{


    TEST_CLASS (Vector2Tests)
    {
        public:
        TEST_METHOD (Vector2_DefaultConstructor_InitializesToZero)
        {
            Vector2 v;
            Assert::AreEqual (0.0f, v.x);
            Assert::AreEqual (0.0f, v.y);
        }






        TEST_METHOD (Vector2_ParameterizedConstructor_SetsComponents)
        {
            Vector2 v(3.5f, -2.1f);
            Assert::AreEqual (3.5f, v.x);
            Assert::AreEqual (-2.1f, v.y);
        }






        TEST_METHOD (Math_Vector2Add_ComputesCorrectSum)
        {
            Vector2 a(1.0f, 2.0f);
            Vector2 b(3.0f, 4.0f);
            Vector2 result = Math::Add (a, b);
            Assert::AreEqual (4.0f, result.x);
            Assert::AreEqual (6.0f, result.y);
        }






        TEST_METHOD (Math_Vector2Subtract_ComputesCorrectDifference)
        {
            Vector2 a(5.0f, 8.0f);
            Vector2 b(2.0f, 3.0f);
            Vector2 result = Math::Subtract (a, b);
            Assert::AreEqual (3.0f, result.x);
            Assert::AreEqual (5.0f, result.y);
        }






        TEST_METHOD (Math_Vector2Multiply_ScalesVector)
        {
            Vector2 v(2.0f, -3.0f);
            Vector2 result = Math::Multiply (v, 2.5f);
            Assert::AreEqual (5.0f, result.x);
            Assert::AreEqual (-7.5f, result.y);
        }






        TEST_METHOD (Math_Vector2Dot_ComputesDotProduct)
        {
            Vector2 a(3.0f, 4.0f);
            Vector2 b(2.0f, 1.0f);
            float result = Math::Dot (a, b);
            Assert::AreEqual (10.0f, result); // 3*2 + 4*1 = 10
        }






        TEST_METHOD (Math_Vector2Length_ComputesMagnitude)
        {
            Vector2 v(3.0f, 4.0f);
            float length = Math::Length (v);
            Assert::AreEqual (5.0f, length); // sqrt(9 + 16) = 5
        }






        TEST_METHOD (Math_Vector2Length_ZeroVector_ReturnsZero)
        {
            Vector2 v(0.0f, 0.0f);
            float length = Math::Length (v);
            Assert::AreEqual (0.0f, length);
        }






        TEST_METHOD (Math_Vector2Normalize_ReturnsUnitVector)
        {
            Vector2 v(3.0f, 4.0f);
            Vector2 result = Math::Normalize (v);
            Assert::AreEqual (0.6f, result.x, 0.0001f);
            Assert::AreEqual (0.8f, result.y, 0.0001f);

            // Verify unit length
            float length = Math::Length (result);
            Assert::AreEqual (1.0f, length, 0.0001f);
        }






        TEST_METHOD (Math_Vector2Normalize_ZeroVector_ReturnsZeroVector)
        {
            Vector2 v(0.0f, 0.0f);
            Vector2 result = Math::Normalize (v);
            Assert::AreEqual (0.0f, result.x);
            Assert::AreEqual (0.0f, result.y);
        }
    };



}  // namespace MatrixRainTests






namespace MatrixRainTests
{


    TEST_CLASS (Vector3Tests)
    {
        public:
        TEST_METHOD (Vector3_DefaultConstructor_InitializesToZero)
        {
            Vector3 v;
            Assert::AreEqual (0.0f, v.x);
            Assert::AreEqual (0.0f, v.y);
            Assert::AreEqual (0.0f, v.z);
        }






        TEST_METHOD (Vector3_ParameterizedConstructor_SetsComponents)
        {
            Vector3 v(1.5f, -2.5f, 3.5f);
            Assert::AreEqual (1.5f, v.x);
            Assert::AreEqual (-2.5f, v.y);
            Assert::AreEqual (3.5f, v.z);
        }






        TEST_METHOD (Math_Vector3Add_ComputesCorrectSum)
        {
            Vector3 a(1.0f, 2.0f, 3.0f);
            Vector3 b(4.0f, 5.0f, 6.0f);
            Vector3 result = Math::Add (a, b);
            Assert::AreEqual (5.0f, result.x);
            Assert::AreEqual (7.0f, result.y);
            Assert::AreEqual (9.0f, result.z);
        }






        TEST_METHOD (Math_Vector3Subtract_ComputesCorrectDifference)
        {
            Vector3 a(10.0f, 8.0f, 6.0f);
            Vector3 b(3.0f, 2.0f, 1.0f);
            Vector3 result = Math::Subtract (a, b);
            Assert::AreEqual (7.0f, result.x);
            Assert::AreEqual (6.0f, result.y);
            Assert::AreEqual (5.0f, result.z);
        }






        TEST_METHOD (Math_Vector3Multiply_ScalesVector)
        {
            Vector3 v(2.0f, -3.0f, 4.0f);
            Vector3 result = Math::Multiply (v, 0.5f);
            Assert::AreEqual (1.0f, result.x);
            Assert::AreEqual (-1.5f, result.y);
            Assert::AreEqual (2.0f, result.z);
        }






        TEST_METHOD (Math_Vector3Dot_ComputesDotProduct)
        {
            Vector3 a(1.0f, 2.0f, 3.0f);
            Vector3 b(4.0f, 5.0f, 6.0f);
            float result = Math::Dot (a, b);
            Assert::AreEqual (32.0f, result); // 1*4 + 2*5 + 3*6 = 32
        }






        TEST_METHOD (Math_Vector3Length_ComputesMagnitude)
        {
            Vector3 v(2.0f, 3.0f, 6.0f);
            float length = Math::Length (v);
            Assert::AreEqual (7.0f, length); // sqrt(4 + 9 + 36) = 7
        }






        TEST_METHOD (Math_Vector3Length_ZeroVector_ReturnsZero)
        {
            Vector3 v(0.0f, 0.0f, 0.0f);
            float length = Math::Length (v);
            Assert::AreEqual (0.0f, length);
        }






        TEST_METHOD (Math_Vector3Normalize_ReturnsUnitVector)
        {
            Vector3 v(2.0f, 3.0f, 6.0f);
            Vector3 result = Math::Normalize (v);

            // Verify unit length
            float length = Math::Length (result);
            Assert::AreEqual (1.0f, length, 0.0001f);

            // Verify components
            Assert::AreEqual (2.0f / 7.0f, result.x, 0.0001f);
            Assert::AreEqual (3.0f / 7.0f, result.y, 0.0001f);
            Assert::AreEqual (6.0f / 7.0f, result.z, 0.0001f);
        }






        TEST_METHOD (Math_Vector3Normalize_ZeroVector_ReturnsZeroVector)
        {
            Vector3 v(0.0f, 0.0f, 0.0f);
            Vector3 result = Math::Normalize (v);
            Assert::AreEqual (0.0f, result.x);
            Assert::AreEqual (0.0f, result.y);
            Assert::AreEqual (0.0f, result.z);
        }
    };



}  // namespace MatrixRainTests






namespace MatrixRainTests
{


    TEST_CLASS (Color4Tests)
    {
        public:
        TEST_METHOD (Color4_DefaultConstructor_InitializesToTransparentBlack)
        {
            Color4 c;
            Assert::AreEqual (0.0f, c.r);
            Assert::AreEqual (0.0f, c.g);
            Assert::AreEqual (0.0f, c.b);
            Assert::AreEqual (1.0f, c.a);
        }






        TEST_METHOD (Color4_ParameterizedConstructor_SetsComponents)
        {
            Color4 c(0.5f, 0.6f, 0.7f, 0.8f);
            Assert::AreEqual (0.5f, c.r);
            Assert::AreEqual (0.6f, c.g);
            Assert::AreEqual (0.7f, c.b);
            Assert::AreEqual (0.8f, c.a);
        }






        TEST_METHOD (Color4_ThreeParameterConstructor_DefaultsAlphaToOne)
        {
            Color4 c(0.2f, 0.3f, 0.4f);
            Assert::AreEqual (0.2f, c.r);
            Assert::AreEqual (0.3f, c.g);
            Assert::AreEqual (0.4f, c.b);
            Assert::AreEqual (1.0f, c.a);
        }






        TEST_METHOD (Math_Color4Lerp_InterpolatesAtZero)
        {
            Color4 a(0.0f, 0.0f, 0.0f, 0.0f);
            Color4 b(1.0f, 1.0f, 1.0f, 1.0f);
            Color4 result = Math::Lerp (a, b, 0.0f);
            Assert::AreEqual (0.0f, result.r);
            Assert::AreEqual (0.0f, result.g);
            Assert::AreEqual (0.0f, result.b);
            Assert::AreEqual (0.0f, result.a);
        }






        TEST_METHOD (Math_Color4Lerp_InterpolatesAtOne)
        {
            Color4 a(0.0f, 0.0f, 0.0f, 0.0f);
            Color4 b(1.0f, 1.0f, 1.0f, 1.0f);
            Color4 result = Math::Lerp (a, b, 1.0f);
            Assert::AreEqual (1.0f, result.r);
            Assert::AreEqual (1.0f, result.g);
            Assert::AreEqual (1.0f, result.b);
            Assert::AreEqual (1.0f, result.a);
        }






        TEST_METHOD (Math_Color4Lerp_InterpolatesAtHalf)
        {
            Color4 a(0.0f, 0.2f, 0.4f, 0.6f);
            Color4 b(1.0f, 0.8f, 0.6f, 1.0f);
            Color4 result = Math::Lerp (a, b, 0.5f);
            Assert::AreEqual (0.5f, result.r);
            Assert::AreEqual (0.5f, result.g);
            Assert::AreEqual (0.5f, result.b);
            Assert::AreEqual (0.8f, result.a);
        }






        TEST_METHOD (Math_Color4Lerp_ClampsNegativeT)
        {
            Color4 a(0.5f, 0.5f, 0.5f, 0.5f);
            Color4 b(1.0f, 1.0f, 1.0f, 1.0f);
            Color4 result = Math::Lerp (a, b, -0.5f);
            // Should clamp to t=0, returning a
            Assert::AreEqual (0.5f, result.r);
            Assert::AreEqual (0.5f, result.g);
            Assert::AreEqual (0.5f, result.b);
            Assert::AreEqual (0.5f, result.a);
        }






        TEST_METHOD (Math_Color4Lerp_ClampsExcessiveT)
        {
            Color4 a(0.0f, 0.0f, 0.0f, 0.0f);
            Color4 b(0.5f, 0.5f, 0.5f, 0.5f);
            Color4 result = Math::Lerp (a, b, 1.5f);
            // Should clamp to t=1, returning b
            Assert::AreEqual (0.5f, result.r);
            Assert::AreEqual (0.5f, result.g);
            Assert::AreEqual (0.5f, result.b);
            Assert::AreEqual (0.5f, result.a);
        }
    };



}  // namespace MatrixRainTests






namespace MatrixRainTests
{


    TEST_CLASS (Matrix4x4Tests)
    {
        public:
        TEST_METHOD (Matrix4x4_DefaultConstructor_CreatesIdentityMatrix)
        {
            Matrix4x4 m;

            // Diagonal should be 1.0
            Assert::AreEqual (1.0f, m.m[0][0]);
            Assert::AreEqual (1.0f, m.m[1][1]);
            Assert::AreEqual (1.0f, m.m[2][2]);
            Assert::AreEqual (1.0f, m.m[3][3]);

            // Off-diagonal should be 0.0
            Assert::AreEqual (0.0f, m.m[0][1]);
            Assert::AreEqual (0.0f, m.m[0][2]);
            Assert::AreEqual (0.0f, m.m[1][0]);
            Assert::AreEqual (0.0f, m.m[2][0]);
        }






        TEST_METHOD (Matrix4x4_CreateOrthographic_CreatesValidProjectionMatrix)
        {
            // Standard orthographic projection for 1920x1080 window
            Matrix4x4 m = Matrix4x4::CreateOrthographic(0.0f, 1920.0f, 0.0f, 1080.0f, 0.0f, 100.0f);

            // Check that matrix is not identity
            Assert::AreNotEqual (1.0f, m.m[0][0]);

            // Verify key elements of orthographic projection
            // m[0][0] = 2 / (right - left)
            Assert::AreEqual (2.0f / 1920.0f, m.m[0][0], 0.0001f);

            // m[1][1] = 2 / (top - bottom)
            Assert::AreEqual (2.0f / 1080.0f, m.m[1][1], 0.0001f);

            // m[2][2] = 1 / (far - near)
            Assert::AreEqual (1.0f / 100.0f, m.m[2][2], 0.0001f);

            // m[3][3] should be 1.0
            Assert::AreEqual (1.0f, m.m[3][3]);
        }






        TEST_METHOD (Matrix4x4_CreateOrthographic_SymmetricBounds_CentersOrigin)
        {
            // Symmetric bounds should place origin at center
            Matrix4x4 m = Matrix4x4::CreateOrthographic(-10.0f, 10.0f, -10.0f, 10.0f, 0.0f, 100.0f);

            // Translation components should be zero for symmetric bounds
            Assert::AreEqual (0.0f, m.m[3][0], 0.0001f);
            Assert::AreEqual (0.0f, m.m[3][1], 0.0001f);
        }
    };



}  // namespace MatrixRainTests






namespace MatrixRainTests
{


    TEST_CLASS (MathUtilityTests)
    {
        public:
        TEST_METHOD (Math_Clamp_ValueBelowMin_ReturnsMin)
        {
            float result = Math::Clamp (-5.0f, 0.0f, 10.0f);
            Assert::AreEqual (0.0f, result);
        }






        TEST_METHOD (Math_Clamp_ValueAboveMax_ReturnsMax)
        {
            float result = Math::Clamp (15.0f, 0.0f, 10.0f);
            Assert::AreEqual (10.0f, result);
        }






        TEST_METHOD (Math_Clamp_ValueInRange_ReturnsValue)
        {
            float result = Math::Clamp (5.0f, 0.0f, 10.0f);
            Assert::AreEqual (5.0f, result);
        }






        TEST_METHOD (Math_Clamp_ValueAtMin_ReturnsMin)
        {
            float result = Math::Clamp (0.0f, 0.0f, 10.0f);
            Assert::AreEqual (0.0f, result);
        }






        TEST_METHOD (Math_Clamp_ValueAtMax_ReturnsMax)
        {
            float result = Math::Clamp (10.0f, 0.0f, 10.0f);
            Assert::AreEqual (10.0f, result);
        }






        TEST_METHOD (Math_FloatLerp_InterpolatesAtZero)
        {
            float result = Math::Lerp (0.0f, 10.0f, 0.0f);
            Assert::AreEqual (0.0f, result);
        }






        TEST_METHOD (Math_FloatLerp_InterpolatesAtOne)
        {
            float result = Math::Lerp (0.0f, 10.0f, 1.0f);
            Assert::AreEqual (10.0f, result);
        }






        TEST_METHOD (Math_FloatLerp_InterpolatesAtHalf)
        {
            float result = Math::Lerp (0.0f, 10.0f, 0.5f);
            Assert::AreEqual (5.0f, result);
        }






        TEST_METHOD (Math_FloatLerp_InterpolatesNegativeRange)
        {
            float result = Math::Lerp (-10.0f, 10.0f, 0.25f);
            Assert::AreEqual (-5.0f, result);
        }






        TEST_METHOD (Math_FloatLerp_AllowsExtrapolation)
        {
            // Note: float lerp doesn't clamp like color lerp
            float result = Math::Lerp (0.0f, 10.0f, 1.5f);
            Assert::AreEqual (15.0f, result);
        }
    };



}  // namespace MatrixRainTests

