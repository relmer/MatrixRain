#include "pch.h"
#include "MatrixRain/Viewport.h"





void Viewport::Resize (float width, float height)
{
    m_width  = width;
    m_height = height;
    
    UpdateProjectionMatrix();
}





float Viewport::GetAspectRatio() const
{
    if (m_height <= 0.0f)
    {
        return 1.0f; // Safe default
    }

    return m_width / m_height;
}





void Viewport::UpdateProjectionMatrix()
{
    // Create orthographic projection for screen-space rendering
    // Maps [0, width] × [0, height] to NDC [-1, 1] × [-1, 1]
    // 
    // For DirectX (left-handed, Y-down screen coordinates):
    // - X: 0 (left) to width (right) → -1 to +1
    // - Y: 0 (top) to height (bottom) → +1 to -1 (inverted for screen space)
    // - Z: 0 (near) to 100 (far) → 0 to 1 (DirectX depth range)
    
    if (m_width <= 0.0f || m_height <= 0.0f)
    {
        // For zero dimensions, set identity matrix
        m_projectionMatrix = Matrix4x4();
        return;
    }

    // Standard orthographic projection formula:
    // X_ndc = 2 * X_screen / width - 1
    // Y_ndc = -(2 * Y_screen / height - 1) = 1 - 2 * Y_screen / height
    //
    // Matrix form (column-major for DirectX):
    // [2/w   0     0    -1  ]
    // [0    -2/h   0     1  ]
    // [0     0    1/100  0  ]
    // [0     0     0     1  ]

    constexpr float nearZ = 0.0f;
    constexpr float farZ = 100.0f;

    m_projectionMatrix.m[0][0] = 2.0f / m_width;
    m_projectionMatrix.m[0][1] = 0.0f;
    m_projectionMatrix.m[0][2] = 0.0f;
    m_projectionMatrix.m[0][3] = 0.0f;

    m_projectionMatrix.m[1][0] = 0.0f;
    m_projectionMatrix.m[1][1] = -2.0f / m_height;
    m_projectionMatrix.m[1][2] = 0.0f;
    m_projectionMatrix.m[1][3] = 0.0f;

    m_projectionMatrix.m[2][0] = 0.0f;
    m_projectionMatrix.m[2][1] = 0.0f;
    m_projectionMatrix.m[2][2] = 1.0f / (farZ - nearZ);
    m_projectionMatrix.m[2][3] = 0.0f;

    m_projectionMatrix.m[3][0] = -1.0f;
    m_projectionMatrix.m[3][1] = 1.0f;
    m_projectionMatrix.m[3][2] = -nearZ / (farZ - nearZ);
    m_projectionMatrix.m[3][3] = 1.0f;
}
