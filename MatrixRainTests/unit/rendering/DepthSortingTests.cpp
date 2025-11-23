#include "pch.h"


#include "MatrixRain/CharacterStreak.h"


using namespace MatrixRain;
namespace MatrixRainTests
{
	TEST_CLASS(DepthSortingTests)
	{
	public:
		
		TEST_METHOD(TestDepthSortingBackToFront)
		{
			// T045: Test depth sorting back-to-front order
			// 
			// Given: Multiple character streaks at different Z depths
			// When: Sorted using the same algorithm as RenderSystem::SortStreaksByDepth
			// Then: Streaks are ordered from farthest (highest Z) to nearest (lowest Z)
			//
			// This ensures proper painter's algorithm for rendering with alpha blending
			
			// Create streaks with known Z positions
			CharacterStreak streak1, streak2, streak3;
			streak1.Spawn(Vector3(0.0f, 0.0f, 10.0f)); // Near
			streak2.Spawn(Vector3(0.0f, 0.0f, 90.0f)); // Far
			streak3.Spawn(Vector3(0.0f, 0.0f, 50.0f)); // Middle
			
			// Create vector of pointers in unsorted order
			std::vector<const CharacterStreak*> streaks;
			streaks.push_back(&streak1);
			streaks.push_back(&streak2);
			streaks.push_back(&streak3);
			
			// Sort using the same algorithm as RenderSystem (back-to-front)
			std::stable_sort(streaks.begin(), streaks.end(),
				[](const CharacterStreak* a, const CharacterStreak* b)
				{
					return a->GetPosition().z > b->GetPosition().z;
				});
			
			// Verify order is back-to-front (far to near): Z=90, Z=50, Z=10
			Assert::AreEqual(90.0f, streaks[0]->GetPosition().z, L"First streak should be farthest (Z=90)");
			Assert::AreEqual(50.0f, streaks[1]->GetPosition().z, L"Second streak should be middle (Z=50)");
			Assert::AreEqual(10.0f, streaks[2]->GetPosition().z, L"Third streak should be nearest (Z=10)");
		}

		TEST_METHOD(TestDepthSortingWithEqualDepths)
		{
			// Edge case: Streaks at same depth should maintain stable order
			
			// Create streaks with same Z but different spawn order
			CharacterStreak streak1, streak2, streak3;
			streak1.Spawn(Vector3(10.0f, 0.0f, 50.0f)); // Same Z, X=10
			streak2.Spawn(Vector3(20.0f, 0.0f, 50.0f)); // Same Z, X=20
			streak3.Spawn(Vector3(30.0f, 0.0f, 50.0f)); // Same Z, X=30
			
			// Create vector in original order
			std::vector<const CharacterStreak*> streaks;
			streaks.push_back(&streak1);
			streaks.push_back(&streak2);
			streaks.push_back(&streak3);
			
			// Sort using the same algorithm as RenderSystem
			std::stable_sort(streaks.begin(), streaks.end(),
				[](const CharacterStreak* a, const CharacterStreak* b)
				{
					return a->GetPosition().z > b->GetPosition().z;
				});
			
			// Verify std::stable_sort preserves original relative order
			Assert::AreEqual(10.0f, streaks[0]->GetPosition().x, L"First streak should remain first (X=10)");
			Assert::AreEqual(20.0f, streaks[1]->GetPosition().x, L"Second streak should remain second (X=20)");
			Assert::AreEqual(30.0f, streaks[2]->GetPosition().x, L"Third streak should remain third (X=30)");
		}

		TEST_METHOD(TestDepthSortingEmptyList)
		{
			// Edge case: Empty streak list should not crash
			
			// Create empty vector
			std::vector<const CharacterStreak*> streaks;
			
			// Sort using the same algorithm as RenderSystem
			std::stable_sort(streaks.begin(), streaks.end(),
				[](const CharacterStreak* a, const CharacterStreak* b)
				{
					return a->GetPosition().z > b->GetPosition().z;
				});
			
			// Verify no crash and still empty
			Assert::AreEqual(static_cast<size_t>(0), streaks.size(), L"Empty vector should remain empty");
		}
	};
}
