#include "pch.h"

#include "FrameMetrics.h"





//  Rec.709 luminance weights, the same primaries the rain is authored and
//  displayed in.
static constexpr float kLumaR       = 0.2126f;
static constexpr float kLumaG       = 0.7152f;
static constexpr float kLumaB       = 0.0722f;

//  Width of the rings the halo profile is averaged over. Half a pixel is fine
//  enough that the curvature of a glow within one ring is negligible, and wide
//  enough that every ring past the centre holds several pixels.
static constexpr float kRingWidthPx = 0.5f;

//  The fraction of the peak that defines the reported radius.
static constexpr float kHalfMaximum = 0.5f;





////////////////////////////////////////////////////////////////////////////////
//
//  SrgbToLinearLocal
//
//  IEC 61966-2-1 electro-optical transfer function. Local until ColorMath
//  lands (task T006), which this then defers to.
//
////////////////////////////////////////////////////////////////////////////////

static float SrgbToLinearLocal (float value) noexcept
{
    if (value <= 0.04045f)
    {
        return value / 12.92f;
    }

    return std::pow ((value + 0.055f) / 1.055f, 2.4f);
}





////////////////////////////////////////////////////////////////////////////////
//
//  LuminanceAt
//
//  Linear Rec.709 luminance of one pixel of an 8-bit BGRA frame.
//
////////////////////////////////////////////////////////////////////////////////

static float LuminanceAt (std::span<const uint8_t> bgra, size_t pixelIndex) noexcept
{
    const size_t offset = pixelIndex * 4;

    const float  blue   = SrgbToLinearLocal (bgra[offset + 0] / 255.0f);
    const float  green  = SrgbToLinearLocal (bgra[offset + 1] / 255.0f);
    const float  red    = SrgbToLinearLocal (bgra[offset + 2] / 255.0f);



    return kLumaR * red + kLumaG * green + kLumaB * blue;
}





////////////////////////////////////////////////////////////////////////////////
//
//  LuminanceAt
//
//  Linear Rec.709 luminance of one pixel of a linear float RGBA frame.
//
////////////////////////////////////////////////////////////////////////////////

static float LuminanceAt (std::span<const float> rgba, size_t pixelIndex) noexcept
{
    const size_t offset = pixelIndex * 4;



    return kLumaR * rgba[offset + 0] + kLumaG * rgba[offset + 1] + kLumaB * rgba[offset + 2];
}





////////////////////////////////////////////////////////////////////////////////
//
//  HasEnoughSamples
//
////////////////////////////////////////////////////////////////////////////////

template <typename TSample>
static bool HasEnoughSamples (std::span<const TSample> samples, UINT width, UINT height) noexcept
{
    const size_t required = static_cast<size_t> (width) * static_cast<size_t> (height) * 4;



    return width > 0 && height > 0 && samples.size() >= required;
}





////////////////////////////////////////////////////////////////////////////////
//
//  MeanLuminanceImpl
//
////////////////////////////////////////////////////////////////////////////////

template <typename TSample>
static float MeanLuminanceImpl (std::span<const TSample> samples, UINT width, UINT height) noexcept
{
    const size_t pixelCount = static_cast<size_t> (width) * static_cast<size_t> (height);
    double       total      = 0.0;


    if (!HasEnoughSamples (samples, width, height))
    {
        return 0.0f;
    }

    for (size_t pixel = 0; pixel < pixelCount; ++pixel)
    {
        total += LuminanceAt (samples, pixel);
    }

    return static_cast<float> (total / static_cast<double> (pixelCount));
}





////////////////////////////////////////////////////////////////////////////////
//
//  BrightestPixelImpl
//
////////////////////////////////////////////////////////////////////////////////

template <typename TSample>
static POINT BrightestPixelImpl (std::span<const TSample> samples,
                                 UINT                     width,
                                 UINT                     height,
                                 float                  * pOutLuminance) noexcept
{
    POINT brightest = { 0, 0 };
    float peak      = -1.0f;


    if (HasEnoughSamples (samples, width, height))
    {
        for (UINT y = 0; y < height; ++y)
        {
            for (UINT x = 0; x < width; ++x)
            {
                const float luminance = LuminanceAt (samples, static_cast<size_t> (y) * width + x);

                if (luminance > peak)
                {
                    peak        = luminance;
                    brightest.x = static_cast<LONG> (x);
                    brightest.y = static_cast<LONG> (y);
                }
            }
        }
    }

    if (pOutLuminance)
    {
        *pOutLuminance = std::max (peak, 0.0f);
    }

    return brightest;
}





////////////////////////////////////////////////////////////////////////////////
//
//  HaloFalloffRadiusImpl
//
//  Builds a radial luminance profile in half-pixel rings and reports where it
//  first crosses half of the centre luminance, interpolated between the two
//  rings that straddle the crossing.
//
////////////////////////////////////////////////////////////////////////////////

template <typename TSample>
static float HaloFalloffRadiusImpl (std::span<const TSample> samples,
                                    UINT                     width,
                                    UINT                     height,
                                    POINT                    center) noexcept
{
    std::vector<double> ringTotal;
    std::vector<size_t> ringCount;
    size_t              ringLimit = 0;
    float               peak      = 0.0f;
    float               previousR = 0.0f;
    float               previousL = 0.0f;


    if (!HasEnoughSamples (samples, width, height))
    {
        return 0.0f;
    }

    if (center.x < 0 || center.y < 0
        || static_cast<UINT> (center.x) >= width || static_cast<UINT> (center.y) >= height)
    {
        return 0.0f;
    }

    //  Only rings that fit entirely inside the frame describe the halo; past
    //  the nearest edge a ring would sample the corners alone.
    {
        const LONG toEdgeX = std::min (center.x, static_cast<LONG> (width)  - 1 - center.x);
        const LONG toEdgeY = std::min (center.y, static_cast<LONG> (height) - 1 - center.y);
        const LONG toEdge  = std::max<LONG> (std::min (toEdgeX, toEdgeY), 0);

        ringLimit = static_cast<size_t> (static_cast<float> (toEdge) / kRingWidthPx) + 1;
    }

    ringTotal.assign (ringLimit, 0.0);
    ringCount.assign (ringLimit, 0);

    for (UINT y = 0; y < height; ++y)
    {
        for (UINT x = 0; x < width; ++x)
        {
            const float  dx     = static_cast<float> (static_cast<LONG> (x) - center.x);
            const float  dy     = static_cast<float> (static_cast<LONG> (y) - center.y);
            const float  radius = std::sqrt (dx * dx + dy * dy);
            const size_t ring   = static_cast<size_t> (radius / kRingWidthPx + 0.5f);

            if (ring < ringLimit)
            {
                ringTotal[ring] += LuminanceAt (samples, static_cast<size_t> (y) * width + x);
                ringCount[ring] += 1;
            }
        }
    }

    if (ringCount.empty() || ringCount[0] == 0)
    {
        return 0.0f;
    }

    peak      = static_cast<float> (ringTotal[0] / static_cast<double> (ringCount[0]));
    previousL = peak;

    if (!(peak > 0.0f))
    {
        return 0.0f;
    }

    for (size_t ring = 1; ring < ringLimit; ++ring)
    {
        if (ringCount[ring] == 0)
        {
            continue;
        }

        const float radius    = static_cast<float> (ring) * kRingWidthPx;
        const float luminance = static_cast<float> (ringTotal[ring] / static_cast<double> (ringCount[ring]));

        if (luminance <= peak * kHalfMaximum)
        {
            //  Straight-line interpolation between the last ring above half
            //  and this one, so the radius is not quantised to the ring width.
            const float span = previousL - luminance;

            if (!(span > 0.0f))
            {
                return radius;
            }

            return previousR + (previousL - peak * kHalfMaximum) / span * (radius - previousR);
        }

        previousR = radius;
        previousL = luminance;
    }

    //  The halo never falls to half inside the frame, so there is no radius to
    //  report: the frame is too small for the glow being measured.
    return 0.0f;
}





////////////////////////////////////////////////////////////////////////////////
//
//  MeanLuminance
//
////////////////////////////////////////////////////////////////////////////////

float MeanLuminance (std::span<const uint8_t> bgra, UINT width, UINT height) noexcept
{
    return MeanLuminanceImpl (bgra, width, height);
}





////////////////////////////////////////////////////////////////////////////////
//
//  MeanLuminance
//
////////////////////////////////////////////////////////////////////////////////

float MeanLuminance (std::span<const float> rgba, UINT width, UINT height) noexcept
{
    return MeanLuminanceImpl (rgba, width, height);
}





////////////////////////////////////////////////////////////////////////////////
//
//  BrightestPixel
//
////////////////////////////////////////////////////////////////////////////////

POINT BrightestPixel (std::span<const uint8_t> bgra, UINT width, UINT height, float * pOutLuminance) noexcept
{
    return BrightestPixelImpl (bgra, width, height, pOutLuminance);
}





////////////////////////////////////////////////////////////////////////////////
//
//  BrightestPixel
//
////////////////////////////////////////////////////////////////////////////////

POINT BrightestPixel (std::span<const float> rgba, UINT width, UINT height, float * pOutLuminance) noexcept
{
    return BrightestPixelImpl (rgba, width, height, pOutLuminance);
}





////////////////////////////////////////////////////////////////////////////////
//
//  HaloFalloffRadius
//
////////////////////////////////////////////////////////////////////////////////

float HaloFalloffRadius (std::span<const uint8_t> bgra, UINT width, UINT height, POINT center) noexcept
{
    return HaloFalloffRadiusImpl (bgra, width, height, center);
}





////////////////////////////////////////////////////////////////////////////////
//
//  HaloFalloffRadius
//
////////////////////////////////////////////////////////////////////////////////

float HaloFalloffRadius (std::span<const float> rgba, UINT width, UINT height, POINT center) noexcept
{
    return HaloFalloffRadiusImpl (rgba, width, height, center);
}
