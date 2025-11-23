#include "pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MatrixRainTests
{
	TEST_CLASS(ColorTransitionTests)
	{
	public:
		
		TEST_METHOD(TestLeadingCharacterWhiteColor)
		{
			// T047: Test character color transitions white→green for leading character
			// 
			// Given: A character streak with a leading character (head)
			// When: Character at head position is queried for color
			// Then: Color is white (RGB 255,255,255), not green
			//
			// Spec requirement: Head character renders white, turns green when next position added
			
			// TODO: Implement test
			// 1. Create CharacterStreak with at least one character
			// 2. Get head character (position 0)
			// 3. Verify color is white (1.0, 1.0, 1.0) or similar representation
			
			Assert::Fail(L"Test not yet implemented - write this test first per TDD");
		}

		TEST_METHOD(TestPreviousHeadTurnsGreen)
		{
			// When streak advances, previous head should turn green
			
			// TODO: Implement test
			// 1. Create streak with white head character
			// 2. Advance streak to add new head position
			// 3. Verify previous head is now green (0, 255, 100) in RGB or normalized
			
			Assert::Fail(L"Test not yet implemented");
		}

		TEST_METHOD(TestTrailingCharactersStayGreen)
		{
			// All non-head characters should remain green throughout fade
			
			// TODO: Implement test
			// 1. Create streak with multiple characters (not just head)
			// 2. Verify all trailing characters have green color
			// 3. Update streak, verify trailing chars still green (only brightness changes)
			
			Assert::Fail(L"Test not yet implemented");
		}

		TEST_METHOD(TestColorPersistsThroughFade)
		{
			// Color should remain constant while brightness fades
			
			// TODO: Implement test
			// 1. Get character color at brightness 1.0
			// 2. Update to reduce brightness to 0.5
			// 3. Verify color unchanged (only brightness/alpha modified)
			
			Assert::Fail(L"Test not yet implemented");
		}
	};
}
