#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\RenderParams.h"
#include "..\..\MatrixRainCore\RenderSystem.h"





namespace MatrixRainTests
{


    // T051 (US3, FR-028b, contracts/scanline-shader.md): the scanline post-
    // pass must be bypassed when scanlines are disabled. GlowEnabled must not
    // affect scanlines (the no-glow path routes through m_postBloomTarget so the
    // scanline PS always has an SRV to sample from). The full RenderSystem::Render
    // path requires a D3D device and isn't unit-testable in isolation; the
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


            // Same check for the output transform's b1 register
            // (contracts/output-transform.md).
            TEST_METHOD (OutputTransformCbMatches16ByteB1Layout)
            {
                Assert::AreEqual (size_t {16},
                                  sizeof (OutputTransformCb),
                                  L"OutputTransformCb must be exactly 16 bytes for HLSL b1 register");
                Assert::AreEqual (size_t {16},
                                  alignof (OutputTransformCb),
                                  L"OutputTransformCb must be 16-byte aligned");
            }


            // Field ORDER is part of the contract too: the shader reads these
            // by position, not by name, so a reordering compiles cleanly and
            // then renders nonsense.  Writing one field at a time and reading
            // back the raw bytes is what catches that.
            TEST_METHOD (OutputTransformCbFieldsAreInContractOrder)
            {
                OutputTransformCb cb    = {};
                const uint8_t *   bytes = reinterpret_cast<const uint8_t *> (&cb);

                cb.outputMode    = 1;
                cb.sdrWhiteScale = 2.0f;
                cb.headroom      = 3.0f;
                cb.isFinalPass   = 4;

                Assert::AreEqual (uint32_t {1}, *reinterpret_cast<const uint32_t *> (bytes + 0),
                                  L"outputMode must be first");
                Assert::AreEqual (2.0f, *reinterpret_cast<const float *> (bytes + 4), 0.0f,
                                  L"sdrWhiteScale must be second");
                Assert::AreEqual (3.0f, *reinterpret_cast<const float *> (bytes + 8), 0.0f,
                                  L"headroom must be third");
                Assert::AreEqual (uint32_t {4}, *reinterpret_cast<const uint32_t *> (bytes + 12),
                                  L"isFinalPass must be fourth");
            }
    };


}
