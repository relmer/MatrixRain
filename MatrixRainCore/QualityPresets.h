#pragma once

#include "IAdapterProvider.h"

#include <cstdint>
#include <optional>




////////////////////////////////////////////////////////////////////////////////
//
//  QualityPresets — preset table + state-machine helpers for the graphics
//  quality control surface (User Story 5).
//
//  Per the locked design in contracts/quality-preset-mapping.md:
//   - Three named presets (Low, Medium, High) and a Custom sentinel.
//   - High is calibrated to today's exact rendering so upgrading users
//     who never open the dialog see no visible change.
//   - Glow on/off is owned by the existing Glow Intensity slider:
//     m_glowIntensityPercent == 0 disables the entire bloom pipeline.
//
////////////////////////////////////////////////////////////////////////////////


enum class QualityPreset : int
{
    Low    = 0,
    Medium = 1,
    High   = 2,
    Custom = 3
};




enum class ResolutionDivisor : int
{
    Full    = 1,
    Half    = 2,
    Quarter = 4,
    Eighth  = 8
};




enum class BlurTaps : int
{
    Low    = 5,
    Medium = 9,
    High   = 13
};




struct AdvancedGraphicsValues
{
    int                m_glowIntensityPercent { 100                       };  // 0..200; 0 = glow disabled
    int                m_blurPasses           { 3                         };  // 1..4
    ResolutionDivisor  m_bloomResolutionDivisor { ResolutionDivisor::Half };
    BlurTaps           m_blurTaps             { BlurTaps::High            };
};


bool operator== (const AdvancedGraphicsValues & a, const AdvancedGraphicsValues & b);
bool operator!= (const AdvancedGraphicsValues & a, const AdvancedGraphicsValues & b);




////////////////////////////////////////////////////////////////////////////////
//
//  LookupPresetValues
//
//  Returns the values for a named preset.  Passing Custom is a precondition
//  violation (assert in debug, fall back to High row in release).
//
////////////////////////////////////////////////////////////////////////////////

AdvancedGraphicsValues LookupPresetValues (QualityPreset preset);




////////////////////////////////////////////////////////////////////////////////
//
//  DetectActivePreset
//
//  Returns the named preset whose row exactly matches the given advanced
//  values, or Custom if no named preset matches.
//
////////////////////////////////////////////////////////////////////////////////

QualityPreset DetectActivePreset (const AdvancedGraphicsValues & current);




////////////////////////////////////////////////////////////////////////////////
//
//  ApplyPresetSnap
//
//  Computes new advanced values when the user changes the preset combo:
//   - Named preset            -> the preset's lookup row.
//   - Custom + lastCustom set -> the saved LastCustom values.
//   - Custom + no LastCustom  -> current unchanged.
//
////////////////////////////////////////////////////////////////////////////////

AdvancedGraphicsValues ApplyPresetSnap (QualityPreset                              preset,
                                        const AdvancedGraphicsValues             & current,
                                        const std::optional<AdvancedGraphicsValues> & lastCustom);




////////////////////////////////////////////////////////////////////////////////
//
//  PickDefaultQualityPreset
//
//  First-run heuristic (runs only when no QualityPreset is saved).
//   - Any discrete adapter (>=256 MB dedicated VRAM, not software) -> High
//   - Integrated only, totalMonitorPixels <= 16M                    -> Medium
//   - Integrated only, totalMonitorPixels  > 16M (~ 2x 4K)          -> Low
//
////////////////////////////////////////////////////////////////////////////////

QualityPreset PickDefaultQualityPreset (const std::vector<AdapterInfo> & adapters,
                                        uint64_t                          totalMonitorPixels);


// Heuristic constants — exposed for direct verification in unit tests.
extern const unsigned int kDiscreteVramThresholdMb;
extern const uint64_t     kHeavyTotalPixelsThreshold;
