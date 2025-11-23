#include "pch.h"
#include "MatrixRain/CharacterStreak.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace MatrixRain;

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
			
			// Create a new streak
			CharacterStreak streak;
			streak.Spawn(Vector3(50.0f, 0.0f, 50.0f));
			
			// Get the head character (should be the last one in the vector)
			const auto& characters = streak.GetCharacters();
			Assert::AreEqual(static_cast<size_t>(1), characters.size(), L"Should have 1 character at spawn");
			
			const CharacterInstance& head = characters.back();
			
			// Verify head is marked as such
			Assert::IsTrue(head.isHead, L"Character should be marked as head");
			
			// Verify color is white (1.0, 1.0, 1.0, 1.0)
			Assert::AreEqual(1.0f, head.color.r, 0.01f, L"Head character red component should be 1.0 (white)");
			Assert::AreEqual(1.0f, head.color.g, 0.01f, L"Head character green component should be 1.0 (white)");
			Assert::AreEqual(1.0f, head.color.b, 0.01f, L"Head character blue component should be 1.0 (white)");
		}

		TEST_METHOD(TestPreviousHeadTurnsGreen)
		{
			// When streak advances, previous head should turn green
			
			// Create a streak
			CharacterStreak streak;
			streak.Spawn(Vector3(50.0f, 0.0f, 50.0f));
			
			// Update the streak enough times to add a second character
			// This requires waiting for the drop interval to elapse
			for (int i = 0; i < 10; ++i)
			{
				streak.Update(0.1f, 600.0f); // 0.1s increments
			}
			
			// Should now have at least 2 characters
			const auto& characters = streak.GetCharacters();
			Assert::IsTrue(characters.size() >= 2, L"Should have at least 2 characters after updates");
			
			// The last character should be the new white head
			const CharacterInstance& newHead = characters.back();
			Assert::IsTrue(newHead.isHead, L"Last character should be the new head");
			Assert::AreEqual(1.0f, newHead.color.r, 0.01f, L"New head should be white");
			
			// Previous characters should be green
			for (size_t i = 0; i < characters.size() - 1; ++i)
			{
				const CharacterInstance& greenChar = characters[i];
				Assert::IsFalse(greenChar.isHead, L"Non-head character should not be marked as head");
				Assert::AreEqual(0.0f, greenChar.color.r, 0.01f, L"Previous head red component should be 0.0 (green)");
				Assert::AreEqual(1.0f, greenChar.color.g, 0.01f, L"Previous head green component should be 1.0 (green)");
				Assert::AreEqual(0.0f, greenChar.color.b, 0.01f, L"Previous head blue component should be 0.0 (green)");
			}
		}

		TEST_METHOD(TestTrailingCharactersStayGreen)
		{
			// All non-head characters should remain green throughout fade
			
			// Create a streak and add multiple characters
			CharacterStreak streak;
			streak.Spawn(Vector3(50.0f, 0.0f, 50.0f));
			
			// Update to add multiple characters
			for (int i = 0; i < 20; ++i)
			{
				streak.Update(0.1f, 600.0f);
			}
			
			// Verify we have multiple characters
			const auto& characters = streak.GetCharacters();
			Assert::IsTrue(characters.size() >= 3, L"Should have at least 3 characters");
			
			// Record colors of non-head characters
			std::vector<Color4> initialColors;
			for (size_t i = 0; i < characters.size() - 1; ++i)
			{
				initialColors.push_back(characters[i].color);
			}
			
			// Update again (characters will fade but color should remain)
			for (int i = 0; i < 10; ++i)
			{
				streak.Update(0.1f, 600.0f);
			}
			
			// Verify colors remained green (only brightness changes)
			const auto& updatedCharacters = streak.GetCharacters();
			for (size_t i = 0; i < std::min(initialColors.size(), updatedCharacters.size() - 1); ++i)
			{
				if (!updatedCharacters[i].isHead)
				{
					Assert::AreEqual(0.0f, updatedCharacters[i].color.r, 0.01f, L"Trailing character should remain green (R=0)");
					Assert::AreEqual(1.0f, updatedCharacters[i].color.g, 0.01f, L"Trailing character should remain green (G=1)");
					Assert::AreEqual(0.0f, updatedCharacters[i].color.b, 0.01f, L"Trailing character should remain green (B=0)");
				}
			}
		}

		TEST_METHOD(TestColorPersistsThroughFade)
		{
			// Color should remain constant while brightness fades
			
			// Create a streak with multiple characters
			CharacterStreak streak;
			streak.Spawn(Vector3(50.0f, 0.0f, 50.0f));
			
			// Add characters and wait for fade to start
			for (int i = 0; i < 30; ++i)
			{
				streak.Update(0.2f, 600.0f);
			}
			
			// Get a non-head character that should be fading
			const auto& characters = streak.GetCharacters();
			if (characters.size() > 2)
			{
				const CharacterInstance& fadingChar = characters[0]; // Oldest character
				
				// Record initial state
				Color4 initialColor = fadingChar.color;
				float initialBrightness = fadingChar.brightness;
				
				// Update more to continue fade
				for (int i = 0; i < 10; ++i)
				{
					streak.Update(0.3f, 600.0f);
				}
				
				// Check if the character still exists (might have faded out)
				const auto& updatedCharacters = streak.GetCharacters();
				if (!updatedCharacters.empty() && !updatedCharacters[0].isHead)
				{
					const CharacterInstance& stillFading = updatedCharacters[0];
					
					// Color should be the same (green)
					Assert::AreEqual(initialColor.r, stillFading.color.r, 0.01f, L"Color R should remain constant during fade");
					Assert::AreEqual(initialColor.g, stillFading.color.g, 0.01f, L"Color G should remain constant during fade");
					Assert::AreEqual(initialColor.b, stillFading.color.b, 0.01f, L"Color B should remain constant during fade");
					
					// Brightness should have decreased (or character would have been removed)
					Assert::IsTrue(stillFading.brightness <= initialBrightness, L"Brightness should decrease or stay same during fade");
				}
			}
		}
	};
}
