#include "pch.h"

#include "QualityPresets.h"




// Heuristic constants for first-run preset selection.  Externally visible
// so unit tests can pin them and protect against silent retunes.
const unsigned int kDiscreteVramThresholdMb   = 256;
const uint64_t     kHeavyTotalPixelsThreshold = 16'000'000ull;




////////////////////////////////////////////////////////////////////////////////
//
//  AdvancedGraphicsValues equality
//
////////////////////////////////////////////////////////////////////////////////

bool operator== (const AdvancedGraphicsValues & a, const AdvancedGraphicsValues & b)
{
    return a.m_glowIntensityPercent    == b.m_glowIntensityPercent &&
           a.m_blurPasses              == b.m_blurPasses &&
           a.m_bloomResolutionDivisor  == b.m_bloomResolutionDivisor &&
           a.m_blurTaps                == b.m_blurTaps;
}


bool operator!= (const AdvancedGraphicsValues & a, const AdvancedGraphicsValues & b)
{
    return !(a == b);
}




////////////////////////////////////////////////////////////////////////////////
//
//  LookupPresetValues
//
////////////////////////////////////////////////////////////////////////////////

AdvancedGraphicsValues LookupPresetValues (QualityPreset preset)
{
    switch (preset)
    {
        case QualityPreset::Low:
            return AdvancedGraphicsValues { 75,  1, ResolutionDivisor::Quarter, BlurTaps::Low    };

        case QualityPreset::Medium:
            return AdvancedGraphicsValues { 100, 2, ResolutionDivisor::Half,    BlurTaps::Medium };

        case QualityPreset::High:
            return AdvancedGraphicsValues { 100, 3, ResolutionDivisor::Half,    BlurTaps::High   };

        case QualityPreset::Custom:
        default:
            // Caller precondition violation; return High as a safe default.
            ASSERT (false);
            return AdvancedGraphicsValues { 100, 3, ResolutionDivisor::Half,    BlurTaps::High   };
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  DetectActivePreset
//
////////////////////////////////////////////////////////////////////////////////

QualityPreset DetectActivePreset (const AdvancedGraphicsValues & current)
{
    if (current == LookupPresetValues (QualityPreset::Low))
    {
        return QualityPreset::Low;
    }

    if (current == LookupPresetValues (QualityPreset::Medium))
    {
        return QualityPreset::Medium;
    }

    if (current == LookupPresetValues (QualityPreset::High))
    {
        return QualityPreset::High;
    }

    return QualityPreset::Custom;
}




////////////////////////////////////////////////////////////////////////////////
//
//  ApplyPresetSnap
//
////////////////////////////////////////////////////////////////////////////////

AdvancedGraphicsValues ApplyPresetSnap (QualityPreset                                 preset,
                                        const AdvancedGraphicsValues                & current,
                                        const std::optional<AdvancedGraphicsValues> & lastCustom)
{
    if (preset == QualityPreset::Custom)
    {
        if (lastCustom.has_value())
        {
            return *lastCustom;
        }

        return current;
    }

    return LookupPresetValues (preset);
}




////////////////////////////////////////////////////////////////////////////////
//
//  PickDefaultQualityPreset
//
////////////////////////////////////////////////////////////////////////////////

QualityPreset PickDefaultQualityPreset (const std::vector<AdapterInfo> & adapters,
                                        uint64_t                          totalMonitorPixels)
{
    bool hasDiscrete = false;


    for (const AdapterInfo & adapter : adapters)
    {
        if (!adapter.m_isSoftware && adapter.m_dedicatedVramMb >= kDiscreteVramThresholdMb)
        {
            hasDiscrete = true;
            break;
        }
    }


    if (hasDiscrete)
    {
        return QualityPreset::High;
    }

    if (totalMonitorPixels > kHeavyTotalPixelsThreshold)
    {
        return QualityPreset::Low;
    }

    return QualityPreset::Medium;
}
