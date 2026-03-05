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
                bool foundSlashC = false;
                bool foundSlashQ = false;

                for (const auto & line : lines)
                {
                    if (line.find (L"/c") != std::wstring::npos) foundSlashC = true;
                    if (line.find (L"/?") != std::wstring::npos) foundSlashQ = true;
                }

                Assert::IsTrue (foundSlashC, L"Should contain /c switch");
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

                bool foundDashC = false;
                bool foundDashQ = false;

                for (const auto & line : lines)
                {
                    if (line.find (L"-c") != std::wstring::npos) foundDashC = true;
                    if (line.find (L"-?") != std::wstring::npos) foundDashQ = true;
                }

                Assert::IsTrue (foundDashC, L"Should contain -c switch");
                Assert::IsTrue (foundDashQ, L"Should contain -? switch");
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




            ////////////////////////////////////////////////////////////
            //  T086: Section grouping — GetFormattedText
            ////////////////////////////////////////////////////////////

            TEST_METHOD (GetFormattedText_ContainsOptionsSectionHeader)
            {
                UsageText usage (L'/');


                const std::wstring & text = usage.GetFormattedText();



                Assert::IsTrue (text.find (L"Options:") != std::wstring::npos,
                                L"Should contain 'Options:' section header");
            }




            TEST_METHOD (GetFormattedText_DoesNotContainScreensaverOptions)
            {
                UsageText usage (L'/');


                const std::wstring & text = usage.GetFormattedText();



                Assert::IsTrue (text.find (L"Screensaver Options:") == std::wstring::npos,
                                L"Should not contain 'Screensaver Options:' header");
                Assert::IsTrue (text.find (L"/s") == std::wstring::npos,
                                L"Should not contain /s switch");
                Assert::IsTrue (text.find (L"/p") == std::wstring::npos,
                                L"Should not contain /p switch");
                Assert::IsTrue (text.find (L"/a") == std::wstring::npos,
                                L"Should not contain /a switch");
            }




            TEST_METHOD (GetFormattedText_ContainsVersionString)
            {
                UsageText usage (L'/');


                const std::wstring & text = usage.GetFormattedText();



                Assert::IsTrue (text.find (L"MatrixRain v") != std::wstring::npos,
                                L"Should contain version header");
            }




            TEST_METHOD (GetFormattedText_ContainsCopyrightLine)
            {
                UsageText usage (L'/');


                const std::wstring & text = usage.GetFormattedText();



                Assert::IsTrue (text.find (L"Copyright") != std::wstring::npos,
                                L"Should contain copyright text");
                Assert::IsTrue (text.find (L"Robert Elmer") != std::wstring::npos,
                                L"Should contain author name");
            }




            TEST_METHOD (GetFormattedText_ContainsArchitecture)
            {
                UsageText usage (L'/');


                const std::wstring & text = usage.GetFormattedText();

                // Should contain either x64 or ARM64
                bool hasArch = (text.find (L"x64") != std::wstring::npos) ||
                               (text.find (L"ARM64") != std::wstring::npos);



                Assert::IsTrue (hasArch, L"Should contain architecture string (x64 or ARM64)");
            }




            TEST_METHOD (GetFormattedText_UsesEmDashSeparator)
            {
                UsageText usage (L'/');


                const std::wstring & text = usage.GetFormattedText();



                Assert::IsTrue (text.find (L'\u2014') != std::wstring::npos,
                                L"Should use em dash separator between switch name and description");
            }




            TEST_METHOD (GetFormattedText_DoesNotContainHotkeys)
            {
                UsageText usage (L'/');


                const std::wstring & text = usage.GetFormattedText();



                // Hotkeys like "Space", "Enter", "Esc" as standalone key names should not appear
                // (they may appear in descriptions, so check for typical hotkey-only patterns)
                Assert::IsTrue (text.find (L"Hotkeys:") == std::wstring::npos,
                                L"Should not contain 'Hotkeys:' section");
                Assert::IsTrue (text.find (L"Runtime Hotkeys:") == std::wstring::npos,
                                L"Should not contain 'Runtime Hotkeys:' section");
            }




            TEST_METHOD (GetFormattedText_MatchesFormattedLinesContent)
            {
                UsageText usage (L'/');


                const std::wstring &              text  = usage.GetFormattedText();
                const std::vector<std::wstring> & lines = usage.GetFormattedLines();

                // Rebuild from lines using LF joins to compare
                std::wstring rebuilt;
                for (size_t i = 0; i < lines.size(); i++)
                {
                    rebuilt += lines[i];
                    if (i + 1 < lines.size())
                    {
                        rebuilt += L"\n";
                    }
                }



                Assert::AreEqual (rebuilt, text, L"GetFormattedText() should match LF-joined GetFormattedLines()");
            }
    };




}  // namespace MatrixRainTests
