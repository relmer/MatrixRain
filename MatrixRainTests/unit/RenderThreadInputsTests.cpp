#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\RenderThreadInputs.h"





namespace MatrixRainTests
{


    // MakeRenderThreadInputs only routes pointers by role and never dereferences
    // them, so distinct non-null sentinels are sufficient to assert the wiring.
    static SharedState      * const SHARED_SENTINEL  = reinterpret_cast<SharedState *>      (0x1000);
    static OverlayState     * const OVERLAY_SENTINEL = reinterpret_cast<OverlayState *>     (0x2000);
    static ApplicationState * const APP_SENTINEL     = reinterpret_cast<ApplicationState *> (0x3000);




    TEST_CLASS (RenderThreadInputsTests)
    {
        public:
            TEST_METHOD (Primary_ReceivesOverlaysAndClock)
            {
                std::atomic<bool>  transition { false };

                RenderThreadInputs inputs = MakeRenderThreadInputs (true,
                                                                    SHARED_SENTINEL,
                                                                    OVERLAY_SENTINEL,
                                                                    APP_SENTINEL,
                                                                    &transition);

                Assert::IsTrue (inputs.overlays     == OVERLAY_SENTINEL);
                Assert::IsTrue (inputs.primaryClock == APP_SENTINEL);
            }




            TEST_METHOD (Secondary_ReceivesNoOverlaysOrClock)
            {
                std::atomic<bool>  transition { false };

                RenderThreadInputs inputs = MakeRenderThreadInputs (false,
                                                                    SHARED_SENTINEL,
                                                                    OVERLAY_SENTINEL,
                                                                    APP_SENTINEL,
                                                                    &transition);

                Assert::IsNull (inputs.overlays);
                Assert::IsNull (inputs.primaryClock);
            }




            TEST_METHOD (Primary_BroadcastsSharedStateAndTransition)
            {
                std::atomic<bool>  transition { false };

                RenderThreadInputs inputs = MakeRenderThreadInputs (true,
                                                                    SHARED_SENTINEL,
                                                                    OVERLAY_SENTINEL,
                                                                    APP_SENTINEL,
                                                                    &transition);

                Assert::IsTrue (inputs.sharedState  == SHARED_SENTINEL);
                Assert::IsTrue (inputs.inTransition == &transition);
            }




            TEST_METHOD (Secondary_StillBroadcastsSharedStateAndTransition)
            {
                std::atomic<bool>  transition { false };

                RenderThreadInputs inputs = MakeRenderThreadInputs (false,
                                                                    SHARED_SENTINEL,
                                                                    OVERLAY_SENTINEL,
                                                                    APP_SENTINEL,
                                                                    &transition);

                Assert::IsTrue (inputs.sharedState  == SHARED_SENTINEL);
                Assert::IsTrue (inputs.inTransition == &transition);
            }




            TEST_METHOD (AllContexts_ShareSameSharedStateAndTransition)
            {
                std::atomic<bool>  transition { false };

                RenderThreadInputs primary   = MakeRenderThreadInputs (true,
                                                                      SHARED_SENTINEL,
                                                                      OVERLAY_SENTINEL,
                                                                      APP_SENTINEL,
                                                                      &transition);

                RenderThreadInputs secondary = MakeRenderThreadInputs (false,
                                                                      SHARED_SENTINEL,
                                                                      OVERLAY_SENTINEL,
                                                                      APP_SENTINEL,
                                                                      &transition);

                Assert::IsTrue (primary.sharedState  == secondary.sharedState);
                Assert::IsTrue (primary.inTransition == secondary.inTransition);
            }
    };


}
