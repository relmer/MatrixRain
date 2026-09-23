#ifndef MATRIXRAIN_SHADERS_COLORCONSTANTS_H
#define MATRIXRAIN_SHADERS_COLORCONSTANTS_H

//
//  The sRGB transfer function's constants, in the one form both languages can
//  read.
//
//  These numbers are needed twice: by ColorMath.cpp on the CPU, and by
//  ColorTransfer.hlsli on the GPU, which has to be a line-for-line
//  transliteration of the polynomial curves in ColorMath.cpp. Two copies of a
//  number are a bug waiting to be written, so there is one copy, here, and
//  both sides include it.
//
//  Plain #define rather than typed constants, because this file is compiled by
//  both the C++ compiler and FXC and only the preprocessor behaves identically
//  in both. Do not add anything here that is not a #define.
//
//  IEC 61966-2-1.
//

#define MR_SRGB_ENCODED_KNEE   0.04045f
#define MR_SRGB_LINEAR_KNEE    0.0031308f
#define MR_SRGB_LINEAR_SLOPE   12.92f
#define MR_SRGB_CURVE_OFFSET   0.055f
#define MR_SRGB_CURVE_SCALE    1.055f
#define MR_SRGB_CURVE_GAMMA    2.4f

//
//  Polynomial fits of the two curved segments, for the GPU. pow() is a pair of
//  transcendentals at a quarter of the ALU rate; these are a handful of
//  full-rate multiply-adds. On a desktop GPU the difference is invisible
//  under the memory traffic, but on an integrated GPU the pow() math was
//  measured as nearly the whole cost of the linear-light pipeline (a Surface
//  Pro 8's Iris Xe: +5% to +9% per frame at Medium and High), and on WARP it
//  was most of a +100%.
//
//  Both are minimax fits over each segment's whole domain, verified in 32-bit
//  float arithmetic: decoding lands within 0.013 of an 8-bit code value after
//  an exact re-encode, encoding within 0.084. ColorMath.cpp evaluates the same
//  polynomials on the CPU so a unit test holds them to those bounds.
//
//  Decode: linear = P(encoded) for encoded in [MR_SRGB_ENCODED_KNEE, 1], P a
//  degree-5 polynomial in the encoded value, highest power first.
//

#define MR_SRGB_DECODE_C5      0.0872887142f
#define MR_SRGB_DECODE_C4     -0.307384019f
#define MR_SRGB_DECODE_C3      0.667159148f
#define MR_SRGB_DECODE_C2      0.519384601f
#define MR_SRGB_DECODE_C1      0.0327454925f
#define MR_SRGB_DECODE_C0      0.000916920236f

//
//  Encode: encoded = Q(sqrt(linear)) for linear in [MR_SRGB_LINEAR_KNEE, 1],
//  Q a degree-5 polynomial in the square root, highest power first. The
//  square root is what lets a polynomial follow the curve's near-vertical
//  start; without it the fit needs several segments to do as well.
//

#define MR_SRGB_ENCODE_C5      0.517500998f
#define MR_SRGB_ENCODE_C4     -1.62416267f
#define MR_SRGB_ENCODE_C3      2.03205255f
#define MR_SRGB_ENCODE_C2     -1.40294441f
#define MR_SRGB_ENCODE_C1      1.51799129f
#define MR_SRGB_ENCODE_C0     -0.0401122508f

//
//  Highlights above SDR white (spec 008 research R14). The composite adds the
//  blurred part of the scene above white at this strength times the Glow
//  Intensity slider's multiplier over its default, so the slider scales the
//  highlight glow the way it scales the ordinary one and the default leaves
//  it at 1. Both are used on the GPU; the default is also the C++ slider
//  mapping's anchor (RenderSystem::SetGlowIntensity).
//

#define MR_HIGHLIGHT_GLOW_STRENGTH    1.0f

//
//  Blur passes for the highlight glow. The full glow runs up to four passes
//  of the quality preset's kernel; the part above white is the glow's core,
//  and repeating all of that doubled the blur's cost (0.23 ms at 4K High on
//  the desktop card, T041a). It gets one pass of the 5-tap kernel instead,
//  which keeps it near the glyph, where the core is. C++ only.
//

#define MR_HIGHLIGHT_BLUR_PASSES      1
#define MR_DEFAULT_BLOOM_INTENSITY    2.5f

#endif
