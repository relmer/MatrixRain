#pragma once

#include "Math.h"





/// <summary>
/// Manages viewport dimensions and projection matrix for rendering.
/// Provides orthographic projection for 2D screen-space rendering.
/// </summary>
class Viewport
{
public:
    /// <summary>
    /// Update viewport dimensions and recalculate projection matrix.
    /// </summary>
    /// <param name="width">New width in pixels</param>
    /// <param name="height">New height in pixels</param>
    void Resize (float width, float height);

    // Accessors
    float               GetWidth()             const { return m_width;             }
    float               GetHeight()            const { return m_height;            }
    float               GetAspectRatio()       const;
    const Matrix4x4 &   GetProjectionMatrix()  const { return m_projectionMatrix;  }

private:
    float     m_width             { 0.0f };
    float     m_height            { 0.0f };
    Matrix4x4 m_projectionMatrix;

    /// <summary>
    /// Recalculate orthographic projection matrix based on current dimensions.
    /// Maps screen coordinates [0, width] × [0, height] to NDC [-1, 1] × [-1, 1].
    /// </summary>
    void UpdateProjectionMatrix();
};






