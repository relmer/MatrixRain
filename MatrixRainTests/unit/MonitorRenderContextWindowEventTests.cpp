#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\MonitorRenderContext.h"
#include "..\..\MatrixRainCore\Viewport.h"





namespace MatrixRainTests
{


    //  The UI thread's window events (Resize, OnDpiChanged) apply immediately
    //  while no render thread is running -- Initialize seeds the viewport that
    //  way, and a WM_SIZE that arrives before the thread starts must not be
    //  lost.  Once the thread runs they are handed over instead, which needs a
    //  D3D device to exercise; scripts/Test-DpiDrag.ps1 covers that path on a
    //  real window.
    TEST_CLASS (MonitorRenderContextWindowEventTests)
    {
        public:
            TEST_METHOD (Resize_BeforeThreadStarts_AppliesToTheViewportAtOnce)
            {
                MonitorRenderContext ctx (true);



                ctx.Resize (1280, 720, false);

                Assert::AreEqual (1280.0f, ctx.ViewportRef().GetWidth(),  L"Width must apply synchronously with no render thread");
                Assert::AreEqual (720.0f,  ctx.ViewportRef().GetHeight(), L"Height must apply synchronously with no render thread");
            }


            TEST_METHOD (Resize_BeforeThreadStarts_LatestSizeWins)
            {
                MonitorRenderContext ctx (true);



                ctx.Resize (800,  600,  false);
                ctx.Resize (1920, 1080, false);

                Assert::AreEqual (1920.0f, ctx.ViewportRef().GetWidth(),  L"Second resize must replace the first");
                Assert::AreEqual (1080.0f, ctx.ViewportRef().GetHeight(), L"Second resize must replace the first");
            }


            TEST_METHOD (OnDpiChanged_BeforeThreadStarts_AppliesAtOnce)
            {
                MonitorRenderContext ctx (true);



                ctx.OnDpiChanged (144);

                Assert::AreEqual (1.5f, ctx.GetDpiScale(), 1e-6f, L"144 DPI is a 1.5 scale, applied synchronously with no render thread");
            }
    };
}
