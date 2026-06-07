#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\RenderParams.h"
#include "..\..\MatrixRainCore\RenderSystem.h"





namespace MatrixRainTests
{


    // T036 (US2, FR-015): bloom pipeline must be bypassed when the user
    // toggles Glow Enabled OFF.  The full RenderSystem::Render path requires
    // a D3D device and isn't unit-testable in isolation; we cover the
    // decision predicate via the pure helper ShouldRunBloomPass(params)
    // and trust the impl test (manual UI smoke) to verify the actual
    // bypass branch in RenderSystem.cpp uses it.
    TEST_CLASS (RenderSystemBloomBypassTests)
    {
        public:
            TEST_METHOD (BloomRunsWhenGlowEnabled)
            {
                RenderParams params {};
                params.glowEnabled = true;

                Assert::IsTrue (ShouldRunBloomPass (params),
                                L"glowEnabled=true must run the bloom pipeline");
            }


            TEST_METHOD (BloomBypassedWhenGlowDisabled)
            {
                RenderParams params {};
                params.glowEnabled = false;

                Assert::IsFalse (ShouldRunBloomPass (params),
                                 L"glowEnabled=false must bypass the bloom pipeline (FR-015)");
            }
    };


}
