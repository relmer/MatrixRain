#include "Pch_MatrixRainTests.h"

#include "..\SpyRenderSystem.h"
#include "..\..\MatrixRainCore\IRenderSystem.h"
#include "..\..\MatrixRainCore\AnimationSystem.h"
#include "..\..\MatrixRainCore\Viewport.h"





namespace MatrixRainTests
{


    TEST_CLASS (IRenderSystemTests)
    {
        public:
            TEST_METHOD (Spy_DrivenThroughInterface_RecordsFrameCalls)
            {
                SpyRenderSystem  spy;
                IRenderSystem  & renderer = spy;

                AnimationSystem  animationSystem;
                Viewport         viewport;
                RenderParams     params;
                params.rainPercentage = 42;

                renderer.Render (animationSystem, viewport, params);
                renderer.Render (animationSystem, viewport, params);
                renderer.Present();

                Assert::AreEqual (2, spy.m_renderCount);
                Assert::AreEqual (1, spy.m_presentCount);
                Assert::AreEqual (42, spy.m_lastParams.rainPercentage);
            }




            TEST_METHOD (Spy_DrivenThroughInterface_RecordsBroadcastSettings)
            {
                SpyRenderSystem  spy;
                IRenderSystem  & renderer = spy;

                renderer.SetGlowIntensity        (150);
                renderer.SetGlowSize             (80);
                renderer.SetCharacterScaleOverride (1.25f);
                renderer.Resize                  (1920, 1080);
                renderer.OnDpiChanged            (144);

                Assert::AreEqual (150, spy.m_glowIntensity);
                Assert::AreEqual (80, spy.m_glowSize);
                Assert::AreEqual (1.25f, spy.m_characterScale);
                Assert::AreEqual (UINT (1920), spy.m_lastResizeWidth);
                Assert::AreEqual (UINT (1080), spy.m_lastResizeHeight);
                Assert::AreEqual (UINT (144), spy.m_lastDpi);
            }
    };


}
