#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ScanlineStyleMapping.h"





namespace MatrixRainTests
{


    // The rain ROW PITCH at characterScale == 1.0 (AnimationSystem's
    // BASE_SPACING) -- the true text cell, not the 36px glyph quad.
    static constexpr float kCellPx            = 24.0f;

    // Row pitch at 125% and 150% DPI -- the two cell sizes the calibration
    // was verified on.
    static constexpr float kCell125Px         = 30.0f;
    static constexpr float kCell150Px         = 36.0f;

    // Measured fraction of the row pitch actually covered by glyph ink
    // (~18px of the 24px pitch), used to express expectations the way a
    // viewer counts them: scanlines crossing a visible character.
    static constexpr float kInkFractionOfCell = 18.3f / 24.0f;

    // Heights of a 3840x2160 panel in landscape and rotated into portrait --
    // the configuration that exposed the original defect.
    static constexpr float kLandscapeHeightPx = 2160.0f;
    static constexpr float kPortraitHeightPx  = 3840.0f;





    ////////////////////////////////////////////////////////////////////////////
    //
    //  LinesPerCellOnScreen
    //
    //  Realised scanlines per cell after the round trip through the shader's
    //  lines-per-height uniform.
    //
    ////////////////////////////////////////////////////////////////////////////

    static float LinesPerCellOnScreen (int style, float viewportHeightPx, float cellPx)
    {
        const float lines = ScanlineLineCount (ScanlineLinesPerCell (style), viewportHeightPx, cellPx);



        return lines * cellPx / viewportHeightPx;
    }





    TEST_CLASS (ScanlineStyleMappingTests)
    {
        public:

        ////////////////////////////////////////////////////////////////////////
        // Slider mapping: style -> lines per character cell
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (ScanlineLinesPerCell_AtStyle1_IsAbout10)
        {
            const float lines = ScanlineLinesPerCell (1);
            Assert::IsTrue (std::abs (lines - 9.97f) <= 0.05f,
                            L"Expected ~10 lines/cell at style=1");
        }


        TEST_METHOD (ScanlineLinesPerCell_AtStyle50_IsAbout7Point75)
        {
            const float lines = ScanlineLinesPerCell (50);
            Assert::IsTrue (std::abs (lines - 7.75f) <= 0.05f,
                            L"Expected ~7.75 lines/cell at style=50 (default)");
        }


        TEST_METHOD (ScanlineLinesPerCell_AtStyle100_Is6)
        {
            const float lines = ScanlineLinesPerCell (100);
            Assert::IsTrue (std::abs (lines - 6.0f) <= 0.01f,
                            L"Expected exactly 6 lines/cell at style=100");
        }


        TEST_METHOD (ScanlineLinesPerCell_IsMonotonicallyDecreasing)
        {
            float previous = ScanlineLinesPerCell (1);

            for (int style = 2; style <= 100; ++style)
            {
                const float current = ScanlineLinesPerCell (style);

                Assert::IsTrue (current < previous,
                                L"Raising Style must lower the line density at every step");
                previous = current;
            }
        }


        TEST_METHOD (ScanlineLinesPerCell_StaysInRange)
        {
            for (int style = 1; style <= 100; ++style)
            {
                const float lines = ScanlineLinesPerCell (style);

                Assert::IsTrue (lines >= 6.0f - 0.01f && lines <= 10.0f + 0.01f,
                                L"Every Style position must stay within 6..10 lines/cell");
            }
        }


        TEST_METHOD (ScanlineLinesPerCell_ClampsBelowRange)
        {
            const float linesAt1      = ScanlineLinesPerCell (1);
            const float linesBelow1   = ScanlineLinesPerCell (0);
            const float linesFarBelow = ScanlineLinesPerCell (-50);

            Assert::AreEqual (linesAt1, linesBelow1, 0.001f, L"style=0 must clamp to style=1");
            Assert::AreEqual (linesAt1, linesFarBelow, 0.001f, L"style=-50 must clamp to style=1");
        }


        TEST_METHOD (ScanlineLinesPerCell_ClampsAboveRange)
        {
            const float linesAt100    = ScanlineLinesPerCell (100);
            const float linesAbove100 = ScanlineLinesPerCell (101);
            const float linesFarAbove = ScanlineLinesPerCell (200);

            Assert::AreEqual (linesAt100, linesAbove100, 0.001f, L"style=101 must clamp to style=100");
            Assert::AreEqual (linesAt100, linesFarAbove, 0.001f, L"style=200 must clamp to style=100");
        }


        // Pins the calibration against what a viewer actually counts crossing
        // a glyph, verified on hardware.
        TEST_METHOD (ScanlineLinesPerCell_AcrossGlyphInk_MatchesObservedCounts)
        {
            const float atStyle1   = ScanlineLinesPerCell (1)   * kInkFractionOfCell;
            const float atStyle100 = ScanlineLinesPerCell (100) * kInkFractionOfCell;

            Assert::IsTrue (std::abs (atStyle1 - 7.6f) <= 0.3f,
                            L"Expected ~7.6 scanlines across the glyph ink at style=1");
            Assert::IsTrue (std::abs (atStyle100 - 4.6f) <= 0.3f,
                            L"Expected ~4.6 scanlines across the glyph ink at style=100");
        }


        ////////////////////////////////////////////////////////////////////////
        // Geometry: lines-per-cell -> shader's lines-per-height uniform
        ////////////////////////////////////////////////////////////////////////

        // The regression this change exists to fix. Before, a fixed line count
        // spread across differing heights gave 6.45 lines/cell in landscape
        // and 3.63 in portrait from the same slider position.
        TEST_METHOD (ScanlineLineCount_LinesPerCell_IsIdenticalAcrossMonitorHeights)
        {
            for (int style = 1; style <= 100; ++style)
            {
                const float landscape = LinesPerCellOnScreen (style, kLandscapeHeightPx, kCellPx);
                const float portrait  = LinesPerCellOnScreen (style, kPortraitHeightPx,  kCellPx);

                Assert::AreEqual (landscape, portrait, 0.001f,
                                  L"A portrait and a landscape monitor must show the same lines per cell");
            }
        }


        // Identical character appearance must also hold across DPI: monitors
        // at 125% and 150% have different cell sizes, and must still show the
        // same number of scanlines per cell. This is the property that ruled
        // out integer-pixel pitch snapping.
        TEST_METHOD (ScanlineLineCount_LinesPerCell_IsIdenticalAcrossDpi)
        {
            for (int style = 1; style <= 100; ++style)
            {
                const float at125 = LinesPerCellOnScreen (style, kLandscapeHeightPx, kCell125Px);
                const float at150 = LinesPerCellOnScreen (style, kPortraitHeightPx,  kCell150Px);

                Assert::AreEqual (at125, at150, 0.001f,
                                  L"Monitors at different DPI must show the same lines per cell");
            }
        }


        TEST_METHOD (ScanlineLineCount_LinesPerCell_MatchesTheSliderMapping)
        {
            for (int style = 1; style <= 100; ++style)
            {
                Assert::AreEqual (ScanlineLinesPerCell (style),
                                  LinesPerCellOnScreen (style, kLandscapeHeightPx, kCellPx), 0.001f,
                                  L"Round-tripping through the uniform must preserve lines/cell exactly");
            }
        }


        TEST_METHOD (ScanlineLineCount_ScalesLinearlyWithViewportHeight)
        {
            const float linesPerCell = ScanlineLinesPerCell (50);

            const float atSingle = ScanlineLineCount (linesPerCell, 1080.0f, kCellPx);
            const float atDouble = ScanlineLineCount (linesPerCell, 2160.0f, kCellPx);

            Assert::AreEqual (atSingle * 2.0f, atDouble, 0.001f,
                              L"Doubling the viewport height must double the line count");
        }


        TEST_METHOD (ScanlineLineCount_ScalesInverselyWithCellHeight)
        {
            const float linesPerCell = ScanlineLinesPerCell (50);

            // A half-size cell (characterScale 0.5) must halve the pitch, so
            // the raster stays locked to the glyph on low-height viewports.
            const float atFullCell = ScanlineLineCount (linesPerCell, kLandscapeHeightPx, kCellPx);
            const float atHalfCell = ScanlineLineCount (linesPerCell, kLandscapeHeightPx, kCellPx / 2.0f);

            Assert::AreEqual (atFullCell * 2.0f, atHalfCell, 0.001f,
                              L"Halving the cell height must double the line count");
        }


        TEST_METHOD (ScanlineLineCount_WithZeroCellHeight_DegradesToFlat)
        {
            // Guard path: no glyph metrics available. A 1px pitch is what the
            // shader's sinc roll-off flattens, so this reads as "no scanlines"
            // rather than dividing by zero.
            const float lines = ScanlineLineCount (ScanlineLinesPerCell (50), kLandscapeHeightPx, 0.0f);

            Assert::AreEqual (kLandscapeHeightPx, lines, 0.001f,
                              L"A zero cell height must fall back to a 1px pitch");
        }


        TEST_METHOD (ScanlineLineCount_WithNegativeCellHeight_DegradesToFlat)
        {
            const float lines = ScanlineLineCount (ScanlineLinesPerCell (50), kLandscapeHeightPx, -36.0f);

            Assert::AreEqual (kLandscapeHeightPx, lines, 0.001f,
                              L"A negative cell height must fall back to a 1px pitch");
        }


        ////////////////////////////////////////////////////////////////////////
        // Beat: the pitch bound that keeps scanline gaps from going missing
        ////////////////////////////////////////////////////////////////////////

        // Below ~3px a fractional pitch beats against the pixel grid and some
        // gaps visibly vanish (per-cycle spread 0.42 at 2.27px). The densest
        // Style must keep the smallest verified cell -- 30px at 125% DPI -- at
        // or above 3px. Raising the dense endpoint past 10 breaks this.
        TEST_METHOD (ScanlineLineCount_DensestStyle_KeepsPitchAtLeastThreePixels_On125PercentCell)
        {
            const float lines = ScanlineLineCount (ScanlineLinesPerCell (1), kLandscapeHeightPx, kCell125Px);
            const float pitch = kLandscapeHeightPx / lines;

            Assert::IsTrue (pitch >= 3.0f - 0.02f,
                            L"The densest Style must keep a 30px cell at a pitch of at least 3px");
        }
    };

}
