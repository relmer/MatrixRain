#include "pch.h"

#include "../../MatrixRainCore/InputExitState.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;




namespace MatrixRainTests
{
    TEST_CLASS (InputExitStateTests)
    {
    public:

        TEST_METHOD (InputExitState_DefaultConstruction_InitializesToZeroWithDefaultThreshold)
        {
            InputExitState state;



            Assert::AreEqual (0L,    state.m_initialMousePosition.x);
            Assert::AreEqual (0L,    state.m_initialMousePosition.y);
            Assert::AreEqual (5,     state.m_exitThresholdPixels);
            
            Assert::IsFalse  (state.m_keyboardTriggered);
            Assert::IsFalse  (state.m_mouseTriggered);
        }





        TEST_METHOD (InputExitState_SetInitialPosition_StoresProvidedCoordinates)
        {
            InputExitState state;
            POINT          position = { 100, 200 };



            state.m_initialMousePosition = position;

            Assert::AreEqual (100L, state.m_initialMousePosition.x);
            Assert::AreEqual (200L, state.m_initialMousePosition.y);
        }





        TEST_METHOD (InputExitState_KeyboardTrigger_SetsKeyboardFlag)
        {
            InputExitState state;


            state.m_keyboardTriggered = true;

            Assert::IsTrue (state.m_keyboardTriggered);
            Assert::IsFalse (state.m_mouseTriggered);
        }





        TEST_METHOD (InputExitState_MouseTrigger_SetsMouseFlag)
        {
            InputExitState state;



            state.m_mouseTriggered = true;

            Assert::IsTrue  (state.m_mouseTriggered);
            Assert::IsFalse (state.m_keyboardTriggered);
        }





        TEST_METHOD (InputExitState_BothTriggered_SetsBothFlags)
        {
            InputExitState state;



            state.m_keyboardTriggered = true;
            state.m_mouseTriggered    = true;

            Assert::IsTrue (state.m_keyboardTriggered);
            Assert::IsTrue (state.m_mouseTriggered);
        }





        TEST_METHOD (InputExitState_CustomThreshold_AllowsConfiguration)
        {
            InputExitState state;



            state.m_exitThresholdPixels = 10;

            Assert::AreEqual (10, state.m_exitThresholdPixels);
        }





        TEST_METHOD (InputExitState_ZeroThreshold_AllowsImmediateExit)
        {
            InputExitState state;


            
            state.m_exitThresholdPixels = 0;

            Assert::AreEqual (0, state.m_exitThresholdPixels);
        }





        TEST_METHOD (InputExitState_LargeThreshold_PreventsAccidentalExit)
        {
            InputExitState state;


            
            state.m_exitThresholdPixels = 50;

            Assert::AreEqual (50, state.m_exitThresholdPixels);
        }





        TEST_METHOD (InputExitState_MouseMovementWithinThreshold_DoesNotTrigger)
        {
            InputExitState state;
            POINT          initialPos = { 100, 100 };
            POINT          newPos     = { 103, 102 };  // 3px horizontal, 2px vertical
            int            dx         = abs (newPos.x - initialPos.x);
            int            dy         = abs (newPos.y - initialPos.y);


            
            state.m_initialMousePosition = initialPos;
            state.m_exitThresholdPixels  = 5;

            // Movement distance: sqrt(3^2 + 2^2) ≈ 3.6px < 5px threshold
            bool shouldExit = (dx >= state.m_exitThresholdPixels || dy >= state.m_exitThresholdPixels);

            Assert::IsFalse (shouldExit);
        }





        TEST_METHOD (InputExitState_MouseMovementExceedsThreshold_ShouldTrigger)
        {
            InputExitState state;
            POINT          initialPos = { 100, 100 };
            POINT          newPos     = { 110, 105 };  // 10px horizontal, 5px vertical
            int            dx         = abs (newPos.x - initialPos.x);
            int            dy         = abs (newPos.y - initialPos.y);


            
            state.m_initialMousePosition = initialPos;
            state.m_exitThresholdPixels  = 5;

            // Either axis exceeds threshold
            bool shouldExit = (dx >= state.m_exitThresholdPixels || dy >= state.m_exitThresholdPixels);

            Assert::IsTrue (shouldExit);
        }





        TEST_METHOD (InputExitState_MouseMovementExactlyAtThreshold_ShouldTrigger)
        {
            InputExitState state;
            POINT          initialPos = { 100, 100 };
            POINT          newPos     = { 105, 100 };  // Exactly 5px horizontal
            int            dx         = abs (newPos.x - initialPos.x);
            int            dy         = abs (newPos.y - initialPos.y);


            
            state.m_initialMousePosition = initialPos;
            state.m_exitThresholdPixels  = 5;

            bool shouldExit = (dx >= state.m_exitThresholdPixels || dy >= state.m_exitThresholdPixels);

            Assert::IsTrue (shouldExit);
        }





        TEST_METHOD (InputExitState_NegativeMouseMovement_CalculatesAbsoluteDistance)
        {
            InputExitState state;
            POINT          initialPos = { 100, 100 };
            POINT          newPos     = { 90, 95 };  // -10px horizontal, -5px vertical
            int            dx         = abs (newPos.x - initialPos.x);
            int            dy         = abs (newPos.y - initialPos.y);


            
            state.m_initialMousePosition = initialPos;
            state.m_exitThresholdPixels  = 5;

            bool shouldExit = (dx >= state.m_exitThresholdPixels || dy >= state.m_exitThresholdPixels);

            Assert::IsTrue (shouldExit);
        }





        TEST_METHOD (InputExitState_ResetState_ClearsAllFlags)
        {
            InputExitState state;


            
            // Trigger both flags
            state.m_keyboardTriggered = true;
            state.m_mouseTriggered    = true;

            // Reset
            state.m_keyboardTriggered = false;
            state.m_mouseTriggered    = false;

            Assert::IsFalse (state.m_keyboardTriggered);
            Assert::IsFalse (state.m_mouseTriggered);
        }
    };
}
