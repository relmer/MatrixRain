#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\RenderParams.h"
#include "..\..\MatrixRainCore\RenderSystem.h"





namespace MatrixRainTests
{


    // T051 (US3, FR-028b, contracts/scanline-shader.md): the scanline post-
    // pass must be bypassed when scanlines are disabled OR when glow is
    // disabled (the bloom composite hosts the post-bloom-target plumbing
    // the scanline PS samples from).  The full RenderSystem::Render path
    // requires a D3D device and isn't unit-testable in isolation; the
    // pure-helper predicate carries the decision logic so the impl branch
    // in RenderSystem.cpp can be reviewed against this contract.
    TEST_CLASS (RenderSystemScanlineBypassTests)
    {
        public:
            TEST_METHOD (ScanlineRunsWhenScanlinesAndGlowBothEnabled)
            {
                RenderParams params {};
                params.scanlinesEnabled = true;
                params.glowEnabled      = true;

                Assert::IsTrue (ShouldRunScanlinePass (params),
                                L"scanlines + glow both enabled must run the scanline pass");
            }


            TEST_METHOD (ScanlineBypassedWhenScanlinesDisabled)
            {
                RenderParams params {};
                params.scanlinesEnabled = false;
                params.glowEnabled      = true;

                Assert::IsFalse (ShouldRunScanlinePass (params),
                                 L"scanlinesEnabled=false must bypass the scanline pass");
            }


            TEST_METHOD (ScanlineRunsWhenGlowDisabled)
            {
                RenderParams params {};
                params.scanlinesEnabled = true;
                params.glowEnabled      = false;

                Assert::IsTrue (ShouldRunScanlinePass (params),
                                L"scanlines must still run when only glow is disabled (Render routes the no-glow scene copy through m_postBloomTarget so the scanline PS has an SRV)");
            }


            TEST_METHOD (ScanlineBypassedWhenBothDisabled)
            {
                RenderParams params {};
                params.scanlinesEnabled = false;
                params.glowEnabled      = false;

                Assert::IsFalse (ShouldRunScanlinePass (params),
                                 L"both disabled = no scanline pass");
            }


            // Static-sanity: the CPU mirror struct must match the HLSL b0
            // register layout exactly (16 bytes, two floats + two padding
            // floats).  Same `static_assert` lives in RenderSystem.h; this
            // duplicate-checks via the test runner so a stale build trips
            // an obvious test failure rather than just a compile error.
            TEST_METHOD (ScanlineCbMatches16ByteB0Layout)
            {
                Assert::AreEqual (size_t {16},
                                  sizeof (ScanlineCb),
                                  L"ScanlineCb must be exactly 16 bytes for HLSL b0 register");
                Assert::AreEqual (size_t {16},
                                  alignof (ScanlineCb),
                                  L"ScanlineCb must be 16-byte aligned");
            }
    };


}
