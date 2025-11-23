#include "pch.h"
#include "matrixrain/Math.h"
#include <cmath>

namespace MatrixRain
{
    // Matrix4x4 static method implementation
    Matrix4x4 Matrix4x4::CreateOrthographic(float left, float right, float bottom, float top, float nearZ, float farZ)
    {
        Matrix4x4 result;
        
        // Standard orthographic projection matrix
        result.m[0][0] = 2.0f / (right - left);
        result.m[0][1] = 0.0f;
        result.m[0][2] = 0.0f;
        result.m[0][3] = 0.0f;

        result.m[1][0] = 0.0f;
        result.m[1][1] = 2.0f / (top - bottom);
        result.m[1][2] = 0.0f;
        result.m[1][3] = 0.0f;

        result.m[2][0] = 0.0f;
        result.m[2][1] = 0.0f;
        result.m[2][2] = 1.0f / (farZ - nearZ);
        result.m[2][3] = 0.0f;

        result.m[3][0] = (left + right) / (left - right);
        result.m[3][1] = (top + bottom) / (bottom - top);
        result.m[3][2] = nearZ / (nearZ - farZ);
        result.m[3][3] = 1.0f;

        return result;
    }

    Matrix4x4 Matrix4x4::CreatePerspective(float fovY, float aspectRatio, float nearZ, float farZ)
    {
        Matrix4x4 result;
        
        // Calculate perspective projection matrix
        float tanHalfFovY = std::tan(fovY / 2.0f);
        
        result.m[0][0] = 1.0f / (aspectRatio * tanHalfFovY);
        result.m[0][1] = 0.0f;
        result.m[0][2] = 0.0f;
        result.m[0][3] = 0.0f;

        result.m[1][0] = 0.0f;
        result.m[1][1] = 1.0f / tanHalfFovY;
        result.m[1][2] = 0.0f;
        result.m[1][3] = 0.0f;

        result.m[2][0] = 0.0f;
        result.m[2][1] = 0.0f;
        result.m[2][2] = farZ / (farZ - nearZ);
        result.m[2][3] = 1.0f;

        result.m[3][0] = 0.0f;
        result.m[3][1] = 0.0f;
        result.m[3][2] = -(farZ * nearZ) / (farZ - nearZ);
        result.m[3][3] = 0.0f;

        return result;
    }

    namespace Math
    {
        // Vector2 operations
        Vector2 Add(const Vector2& a, const Vector2& b)
        {
            return Vector2(a.x + b.x, a.y + b.y);
        }

        Vector2 Subtract(const Vector2& a, const Vector2& b)
        {
            return Vector2(a.x - b.x, a.y - b.y);
        }

        Vector2 Multiply(const Vector2& v, float scalar)
        {
            return Vector2(v.x * scalar, v.y * scalar);
        }

        float Dot(const Vector2& a, const Vector2& b)
        {
            return a.x * b.x + a.y * b.y;
        }

        float Length(const Vector2& v)
        {
            return std::sqrt(v.x * v.x + v.y * v.y);
        }

        Vector2 Normalize(const Vector2& v)
        {
            float len = Length(v);
            if (len > 0.0f)
            {
                return Vector2(v.x / len, v.y / len);
            }
            return v;
        }

        // Vector3 operations
        Vector3 Add(const Vector3& a, const Vector3& b)
        {
            return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
        }

        Vector3 Subtract(const Vector3& a, const Vector3& b)
        {
            return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
        }

        Vector3 Multiply(const Vector3& v, float scalar)
        {
            return Vector3(v.x * scalar, v.y * scalar, v.z * scalar);
        }

        float Dot(const Vector3& a, const Vector3& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        float Length(const Vector3& v)
        {
            return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        }

        Vector3 Normalize(const Vector3& v)
        {
            float len = Length(v);
            if (len > 0.0f)
            {
                return Vector3(v.x / len, v.y / len, v.z / len);
            }
            return v;
        }

        // Color operations
        Color4 Lerp(const Color4& a, const Color4& b, float t)
        {
            t = Clamp(t, 0.0f, 1.0f);
            return Color4(
                a.r + (b.r - a.r) * t,
                a.g + (b.g - a.g) * t,
                a.b + (b.b - a.b) * t,
                a.a + (b.a - a.a) * t
            );
        }

        // Utility functions
        float Clamp(float value, float min, float max)
        {
            if (value < min) return min;
            if (value > max) return max;
            return value;
        }

        float Lerp(float a, float b, float t)
        {
            return a + (b - a) * t;
        }
    }
}
