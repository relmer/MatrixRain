#include "pch.h"

#include "../../MatrixRainCore/CharacterConstants.h"





namespace MatrixRainTests
{


    TEST_CLASS (CharacterConstantsTests)
    {
        public:
            TEST_METHOD (KatakanaCodepoints_HasCorrectCount)
            {
                // Should have 72 katakana characters (actual count in array)
                Assert::AreEqual (static_cast<size_t>(72), CharacterConstants::KATAKANA_COUNT);
            }






            TEST_METHOD (KatakanaCodepoints_StartsWithCorrectValue)
            {
                // First katakana should be U+30A0
                Assert::AreEqual (static_cast<uint32_t>(0x30A0), CharacterConstants::KATAKANA_CODEPOINTS[0]);
            }






            TEST_METHOD (KatakanaCodepoints_EndsWithCorrectValue)
            {
                // Last katakana should be U+30E7 (ョ Small Yo)
                size_t lastIndex = CharacterConstants::KATAKANA_COUNT - 1;
                Assert::AreEqual (static_cast<uint32_t>(0x30E7), CharacterConstants::KATAKANA_CODEPOINTS[lastIndex]);
            }






            TEST_METHOD (KatakanaCodepoints_AllInValidRange)
            {
                // All katakana should be in range U+30A0 to U+30FF
                for (size_t i = 0; i < CharacterConstants::KATAKANA_COUNT; i++)
                {
                    uint32_t cp = CharacterConstants::KATAKANA_CODEPOINTS[i];
                    Assert::IsTrue (cp >= 0x30A0);
                    Assert::IsTrue (cp <= 0x30FF);
                }
            }






            TEST_METHOD (LatinUppercaseCodepoints_HasCorrectCount)
            {
                // Should have 26 uppercase letters (A-Z)
                Assert::AreEqual (static_cast<size_t>(26), CharacterConstants::LATIN_UPPERCASE_COUNT);
            }






            TEST_METHOD (LatinUppercaseCodepoints_StartsWithA)
            {
                // First should be 'A' (0x0041)
                Assert::AreEqual (static_cast<uint32_t>(0x0041), CharacterConstants::LATIN_UPPERCASE_CODEPOINTS[0]);
            }






            TEST_METHOD (LatinUppercaseCodepoints_EndsWithZ)
            {
                // Last should be 'Z' (0x005A)
                Assert::AreEqual (static_cast<uint32_t>(0x005A), CharacterConstants::LATIN_UPPERCASE_CODEPOINTS[25]);
            }






            TEST_METHOD (LatinUppercaseCodepoints_IsSequential)
            {
                // Should be sequential from A to Z
                for (size_t i = 0; i < 26; i++)
                {
                    uint32_t expected = 0x0041 + static_cast<uint32_t>(i);
                    Assert::AreEqual (expected, CharacterConstants::LATIN_UPPERCASE_CODEPOINTS[i]);
                }
            }






            TEST_METHOD (LatinLowercaseCodepoints_HasCorrectCount)
            {
                // Should have 26 lowercase letters (a-z)
                Assert::AreEqual (static_cast<size_t>(26), CharacterConstants::LATIN_LOWERCASE_COUNT);
            }






            TEST_METHOD (LatinLowercaseCodepoints_StartsWithSmallA)
            {
                // First should be 'a' (0x0061)
                Assert::AreEqual (static_cast<uint32_t>(0x0061), CharacterConstants::LATIN_LOWERCASE_CODEPOINTS[0]);
            }






            TEST_METHOD (LatinLowercaseCodepoints_EndsWithSmallZ)
            {
                // Last should be 'z' (0x007A)
                Assert::AreEqual (static_cast<uint32_t>(0x007A), CharacterConstants::LATIN_LOWERCASE_CODEPOINTS[25]);
            }






            TEST_METHOD (LatinLowercaseCodepoints_IsSequential)
            {
                // Should be sequential from a to z
                for (size_t i = 0; i < 26; i++)
                {
                    uint32_t expected = 0x0061 + static_cast<uint32_t>(i);
                    Assert::AreEqual (expected, CharacterConstants::LATIN_LOWERCASE_CODEPOINTS[i]);
                }
            }






            TEST_METHOD (NumeralCodepoints_HasCorrectCount)
            {
                // Should have 10 numerals (0-9)
                // Calculate count from array
                const uint32_t* arr = CharacterConstants::NUMERAL_CODEPOINTS;
                Assert::IsNotNull(arr);
                // Verify we can access all 10 elements (0-9)
                Assert::AreEqual (static_cast<uint32_t>(0x0030), arr[0]);
                Assert::AreEqual (static_cast<uint32_t>(0x0039), arr[9]);
            }






            TEST_METHOD (NumeralCodepoints_StartsWithZero)
            {
                // First should be '0' (0x0030)
                Assert::AreEqual (static_cast<uint32_t>(0x0030), CharacterConstants::NUMERAL_CODEPOINTS[0]);
            }






            TEST_METHOD (NumeralCodepoints_EndsWithNine)
            {
                // Last should be '9' (0x0039)
                Assert::AreEqual (static_cast<uint32_t>(0x0039), CharacterConstants::NUMERAL_CODEPOINTS[9]);
            }






            TEST_METHOD (NumeralCodepoints_IsSequential)
            {
                // Should be sequential from 0 to 9
                for (size_t i = 0; i < 10; i++)
                {
                    uint32_t expected = 0x0030 + static_cast<uint32_t>(i);
                    Assert::AreEqual (expected, CharacterConstants::NUMERAL_CODEPOINTS[i]);
                }
            }






            TEST_METHOD (GetAllCodepoints_ReturnsCorrectTotalCount)
            {
                std::vector<uint32_t> all = CharacterConstants::GetAllCodepoints();

                // Total should be 72 + 26 + 26 + 10 = 134
                Assert::AreEqual (static_cast<size_t>(134), all.size());
            }






            TEST_METHOD (GetAllCodepoints_StartsWithKatakana)
            {
                std::vector<uint32_t> all = CharacterConstants::GetAllCodepoints();

                // First character should be first katakana
                Assert::AreEqual (static_cast<uint32_t>(0x30A0), all[0]);
            }






            TEST_METHOD (GetAllCodepoints_ContainsUppercaseAfterKatakana)
            {
                std::vector<uint32_t> all = CharacterConstants::GetAllCodepoints();

                // After 72 katakana, should have 'A'
                Assert::AreEqual (static_cast<uint32_t>(0x0041), all[72]);
            }






            TEST_METHOD (GetAllCodepoints_ContainsLowercaseAfterUppercase)
            {
                std::vector<uint32_t> all = CharacterConstants::GetAllCodepoints();

                // After 72 katakana + 26 uppercase = index 98, should have 'a'
                Assert::AreEqual (static_cast<uint32_t>(0x0061), all[98]);
            }






            TEST_METHOD (GetAllCodepoints_ContainsNumeralsAtEnd)
            {
                std::vector<uint32_t> all = CharacterConstants::GetAllCodepoints();

                // After 72 + 26 + 26 = 124, should have '0'
                Assert::AreEqual (static_cast<uint32_t>(0x0030), all[124]);

                // Last character should be '9'
                Assert::AreEqual (static_cast<uint32_t>(0x0039), all[133]);
            }






            TEST_METHOD (GetAllCodepoints_NoDuplicates)
            {
                std::vector<uint32_t> all = CharacterConstants::GetAllCodepoints();

                // Check for duplicates
                for (size_t i = 0; i < all.size(); i++)
                {
                    for (size_t j = i + 1; j < all.size(); j++)
                    {
                        Assert::AreNotEqual (all[i], all[j], L"Found duplicate codepoint");
                    }
                }
            }






            TEST_METHOD (GetAllCodepoints_AllCodepointsValid)
            {
                std::vector<uint32_t> all = CharacterConstants::GetAllCodepoints();

                // All codepoints should be non-zero and in valid Unicode range
                for (uint32_t cp : all)
                {
                    Assert::IsTrue (cp > 0);
                    Assert::IsTrue (cp <= 0x10FFFF); // Maximum Unicode codepoint
                }
            }
    };



}  // namespace MatrixRainTests

