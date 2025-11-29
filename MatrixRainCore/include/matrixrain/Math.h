#pragma once

namespace MatrixRain
{
    // Vector2: 2D floating-point vector
    struct Vector2
    {
        float x;
        float y;

        Vector2() : x (0.0f), y (0.0f) { }
        Vector2 (float x, float y) : x (x), y (y) { }
    };

    // Vector3: 3D floating-point vector
    struct Vector3
    {
        float x;
        float y;
        float z;

        Vector3() : x (0.0f), y (0.0f), z (0.0f) { }
        Vector3 (float x, float y, float z) : x (x), y (y), z (z) { }
    };

    // Color4: RGBA color with floating-point components (0.0 to 1.0 range)
    struct Color4
    {
        float r;
        float g;
        float b;
        float a;

        constexpr Color4() : r (0.0f), g (0.0f), b (0.0f), a (1.0f) { }
        constexpr Color4 (float r, float g, float b, float a = 1.0f) : r (r), g (g), b (b), a (a) { }
    };

    // Matrix4x4: 4x4 transformation matrix for orthographic projection
    struct Matrix4x4
    {
        float m[4][4];

        Matrix4x4()
        {
            // Initialize to identity matrix
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    m[i][j] = (i == j) ? 1.0f : 0.0f;
                }
            }
        }

        // Create orthographic projection matrix
        // Parameters: left, right, bottom, top, nearZ, farZ
        static Matrix4x4 CreateOrthographic (float left, float right, float bottom, float top, float nearZ, float farZ);
        
        // Create perspective projection matrix
        // Parameters: fovY (field of view in radians), aspect ratio, near plane, far plane
        static Matrix4x4 CreatePerspective (float fovY, float aspectRatio, float nearZ, float farZ);
    };

    // Math utility functions
    namespace Math
    {
        // Vector operations
        Vector2 Add       (const Vector2 & a, const Vector2 & b);
        Vector2 Subtract  (const Vector2 & a, const Vector2 & b);
        Vector2 Multiply  (const Vector2 & v, float scalar);
        float   Dot       (const Vector2 & a, const Vector2 & b);
        float   Length    (const Vector2 & v);
        Vector2 Normalize (const Vector2 & v);

        Vector3 Add       (const Vector3 & a, const Vector3 & b);
        Vector3 Subtract  (const Vector3 & a, const Vector3 & b);
        Vector3 Multiply  (const Vector3 & v, float scalar);
        float   Dot       (const Vector3 & a, const Vector3 & b);
        float   Length    (const Vector3 & v);
        Vector3 Normalize (const Vector3 & v);

        // Color operations
        Color4 Lerp (const Color4 & a, const Color4 & b, float t);
        
        // Constexpr version for compile-time color interpolation
        constexpr Color4 LerpConstexpr (const Color4 & a, const Color4 & b, float t)
        {
            // Simple clamp without calling Clamp() (not constexpr)
            float clamped_t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;
            return Color4 (
                a.r + (b.r - a.r) * clamped_t,
                a.g + (b.g - a.g) * clamped_t,
                a.b + (b.b - a.b) * clamped_t,
                a.a + (b.a - a.a) * clamped_t
            );
        }

        // Utility functions
        float Clamp (float value, float min, float max);
        float Lerp  (float a, float b, float t);
    }
}
