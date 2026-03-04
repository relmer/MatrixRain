#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\UsageText.h"





namespace MatrixRainTests
{




    TEST_CLASS (UsageTextTests)
    {
        public:




            ////////////////////////////////////////////////////////////
            //  T002: UsageText with / prefix
            ////////////////////////////////////////////////////////////

            TEST_METHOD (GetFormattedLines_SlashPrefix_ContainsSlashSwitches)
            {
                UsageText usage (L'/');


                auto lines = usage.GetFormattedLines();



                Assert::IsFalse (lines.empty(), L"Should have formatted lines");

                // Verify switches use / prefix
                bool foundSlashS = false;
                bool foundSlashP = false;
                bool foundSlashC = false;
                bool foundSlashA = false;
                bool foundSlashQ = false;

                for (const auto & line : lines)
                {
                    if (line.find (L"/s") != std::wstring::npos) foundSlashS = true;
                    if (line.find (L"/p") != std::wstring::npos) foundSlashP = true;
                    if (line.find (L"/c") != std::wstring::npos) foundSlashC = true;
                    if (line.find (L"/a") != std::wstring::npos) foundSlashA = true;
                    if (line.find (L"/?") != std::wstring::npos) foundSlashQ = true;
                }

                Assert::IsTrue (foundSlashS, L"Should contain /s switch");
                Assert::IsTrue (foundSlashP, L"Should contain /p switch");
                Assert::IsTrue (foundSlashC, L"Should contain /c switch");
                Assert::IsTrue (foundSlashA, L"Should contain /a switch");
                Assert::IsTrue (foundSlashQ, L"Should contain /? switch");
            }




            TEST_METHOD (GetSwitchPrefix_SlashPrefix_ReturnsSlash)
            {
                UsageText usage (L'/');



                Assert::AreEqual (L'/', usage.GetSwitchPrefix(), L"Prefix should be /");
            }




            ////////////////////////////////////////////////////////////
            //  T003: UsageText with - prefix
            ////////////////////////////////////////////////////////////

            TEST_METHOD (GetFormattedLines_DashPrefix_ContainsDashSwitches)
            {
                UsageText usage (L'-');


                auto lines = usage.GetFormattedLines();



                Assert::IsFalse (lines.empty(), L"Should have formatted lines");

                bool foundDashS = false;
                bool foundDashP = false;
                bool foundDashC = false;

                for (const auto & line : lines)
                {
                    if (line.find (L"-s") != std::wstring::npos) foundDashS = true;
                    if (line.find (L"-p") != std::wstring::npos) foundDashP = true;
                    if (line.find (L"-c") != std::wstring::npos) foundDashC = true;
                }

                Assert::IsTrue (foundDashS, L"Should contain -s switch");
                Assert::IsTrue (foundDashP, L"Should contain -p switch");
                Assert::IsTrue (foundDashC, L"Should contain -c switch");
            }




            TEST_METHOD (GetSwitchPrefix_DashPrefix_ReturnsDash)
            {
                UsageText usage (L'-');



                Assert::AreEqual (L'-', usage.GetSwitchPrefix(), L"Prefix should be -");
            }




            ////////////////////////////////////////////////////////////
            //  T004: GetPlainText returns CRLF-joined, no ANSI
            ////////////////////////////////////////////////////////////

            TEST_METHOD (GetPlainText_ReturnsCRLFJoinedLines)
            {
                UsageText usage (L'/');


                std::wstring plainText = usage.GetPlainText();



                Assert::IsFalse (plainText.empty(), L"Plain text should not be empty");

                // Should contain CRLF
                Assert::IsTrue (plainText.find (L"\r\n") != std::wstring::npos,
                                L"Should contain CRLF line endings");

                // Should NOT contain ANSI escape codes
                Assert::IsTrue (plainText.find (L"\x1b[") == std::wstring::npos,
                                L"Should not contain ANSI escape codes");
            }




            ////////////////////////////////////////////////////////////
            //  T005: DetectSwitchPrefix
            ////////////////////////////////////////////////////////////

            TEST_METHOD (DetectSwitchPrefix_SlashQuestion_ReturnsSlash)
            {
                wchar_t prefix = UsageText::DetectSwitchPrefix (L"/?");



                Assert::AreEqual (L'/', prefix, L"Should detect / prefix from /?");
            }




            TEST_METHOD (DetectSwitchPrefix_DashQuestion_ReturnsDash)
            {
                wchar_t prefix = UsageText::DetectSwitchPrefix (L"-?");



                Assert::AreEqual (L'-', prefix, L"Should detect - prefix from -?");
            }




            TEST_METHOD (DetectSwitchPrefix_NoMatch_DefaultsToSlash)
            {
                wchar_t prefix = UsageText::DetectSwitchPrefix (L"something");



                Assert::AreEqual (L'/', prefix, L"Should default to / when no match");
            }
    };




}  // namespace MatrixRainTests
