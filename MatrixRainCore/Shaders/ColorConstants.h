#ifndef MATRIXRAIN_SHADERS_COLORCONSTANTS_H
#define MATRIXRAIN_SHADERS_COLORCONSTANTS_H

//
//  The sRGB transfer function's constants, in the one form both languages can
//  read.
//
//  These numbers are needed twice: by ColorMath.cpp on the CPU, and by
//  OutputTransform.hlsli on the GPU, which has to be a line-for-line
//  transliteration of LinearToSrgb. Two copies of a number are a bug waiting
//  to be written, so there is one copy, here, and both sides include it.
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

#endif
