#include "pch.h"

#include "FrameMetrics.h"





//  Rec.709 luminance weights, the same primaries the rain is authored and
//  displayed in.
static constexpr float kLumaR            = 0.2126f;
static constexpr float kLumaG            = 0.7152f;
static constexpr float kLumaB            = 0.0722f;

//  Full scale for one channel of an 8-bit frame.
static constexpr float kMaxCodeValue     = 255.0f;

//  Percentile reported alongside the mean, chosen so that a change confined to
//  a small part of the frame -- a ring around every streak head, say -- cannot
//  be averaged away into nothing.
static constexpr float kReportedQuantile = 0.99f;





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

    const float  blue   = SrgbToLinearLocal (bgra[offset + 0] / kMaxCodeValue);
    const float  green  = SrgbToLinearLocal (bgra[offset + 1] / kMaxCodeValue);
    const float  red    = SrgbToLinearLocal (bgra[offset + 2] / kMaxCodeValue);



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
//  CompareFrames
//
////////////////////////////////////////////////////////////////////////////////

FrameDifference CompareFrames (std::span<const uint8_t> baseline,
                               std::span<const uint8_t> candidate,
                               UINT                     width,
                               UINT                     height,
                               float                    threshold) noexcept
{
    const size_t         pixelCount = static_cast<size_t> (width) * static_cast<size_t> (height);
    FrameDifference      difference;
    std::vector<uint8_t> perPixel;
    double               total      = 0.0;


    if (!HasEnoughSamples (baseline, width, height) || !HasEnoughSamples (candidate, width, height))
    {
        return difference;
    }

    //  One byte per pixel holds any difference of 8-bit code values, and keeps
    //  the percentile sort below cheap even on a 4K frame.
    perPixel.resize (pixelCount);

    for (size_t pixel = 0; pixel < pixelCount; ++pixel)
    {
        const size_t offset  = pixel * 4;
        int          largest = 0;


        //  Alpha is deliberately skipped: nothing displays it.
        for (size_t channel = 0; channel < 3; ++channel)
        {
            const int delta = std::abs (static_cast<int> (baseline[offset + channel])
                                        - static_cast<int> (candidate[offset + channel]));

            largest = std::max (largest, delta);
        }

        perPixel[pixel] = static_cast<uint8_t> (largest);
        total          += largest;

        if (static_cast<float> (largest) > difference.m_maxDifference)
        {
            difference.m_maxDifference   = static_cast<float> (largest);
            difference.m_maxDifferenceAt = { static_cast<LONG> (pixel % width),
                                             static_cast<LONG> (pixel / width) };
        }

        if (static_cast<float> (largest) > threshold)
        {
            difference.m_pixelsOverThreshold += 1;
        }
    }

    difference.m_meanDifference = static_cast<float> (total / static_cast<double> (pixelCount));

    {
        size_t index = static_cast<size_t> (static_cast<double> (pixelCount) * kReportedQuantile);

        std::sort (perPixel.begin(), perPixel.end());

        index = std::min (index, pixelCount - 1);

        difference.m_p99Difference = static_cast<float> (perPixel[index]);
    }

    return difference;
}
