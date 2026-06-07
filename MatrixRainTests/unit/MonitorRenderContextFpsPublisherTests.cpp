#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\MonitorRenderContext.h"





namespace MatrixRainTests
{


    // T021 (US1, FR-010): MonitorRenderContext exposes a lock-free FPS
    // publisher pair (m_publishedFps + m_hasPublishedFps) read by the
    // property-sheet 1 Hz title timer.  See contracts/fps-publisher.md.
    TEST_CLASS (MonitorRenderContextFpsPublisherTests)
    {
        public:
            TEST_METHOD (DefaultsToZeroAndUnpublished)
            {
                MonitorRenderContext ctx (true);


                bool   hasValue = true;
                float  value    = 99.0f;


                value    = ctx.GetPublishedFps    (hasValue);
                hasValue = ctx.HasPublishedFps    ();

                Assert::AreEqual (0.0f, value,
                                  L"Default published FPS must be 0.0f");
                Assert::IsFalse  (hasValue,
                                  L"Default HasPublishedFps must be false");
            }


            TEST_METHOD (PublishUpdatesBothFlagsAtomically)
            {
                MonitorRenderContext ctx (true);


                bool   hasValue = false;
                float  value    = 0.0f;


                ctx.PublishFps (60.0f);

                value    = ctx.GetPublishedFps  (hasValue);

                Assert::AreEqual (60.0f, value,
                                  L"Published value must round-trip exactly");
                Assert::IsTrue   (hasValue,
                                  L"HasPublishedFps must be true after first publish");
                Assert::IsTrue   (ctx.HasPublishedFps(),
                                  L"Direct HasPublishedFps accessor must also be true");
            }


            TEST_METHOD (RepublishOverwritesValueAndKeepsFlag)
            {
                MonitorRenderContext ctx (true);


                bool   hasValue = false;
                float  value    = 0.0f;


                ctx.PublishFps (30.5f);
                ctx.PublishFps (144.0f);

                value = ctx.GetPublishedFps (hasValue);

                Assert::AreEqual (144.0f, value,
                                  L"Latest published value wins");
                Assert::IsTrue   (hasValue,
                                  L"HasPublishedFps remains true across republish");
            }
    };


}
