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




            TEST_METHOD (GetFormattedText_ContainsScreensaverOptionsSectionHeader)
            {
                UsageText usage (L'/');


                const std::wstring & text = usage.GetFormattedText();



                Assert::IsTrue (text.find (L"Screensaver Options:") != std::wstring::npos,
                                L"Should contain 'Screensaver Options:' section header");
            }




            TEST_METHOD (GetFormattedText_OptionsSectionBeforeScreensaverSection)
            {
                UsageText usage (L'/');


                const std::wstring & text = usage.GetFormattedText();

                auto optionsPos    = text.find (L"Options:");
                auto screensaverPos = text.find (L"Screensaver Options:");



                Assert::IsTrue (optionsPos != std::wstring::npos, L"Should contain Options:");
                Assert::IsTrue (screensaverPos != std::wstring::npos, L"Should contain Screensaver Options:");
                Assert::IsTrue (optionsPos < screensaverPos, L"Options: should appear before Screensaver Options:");
            }




            TEST_METHOD (GetFormattedText_GeneralSwitchesInOptionsSection)
            {
                UsageText usage (L'/');


                const std::wstring & text = usage.GetFormattedText();

                auto optionsPos     = text.find (L"Options:");
                auto screensaverPos = text.find (L"Screensaver Options:");

                // /c and /? should appear between Options: and Screensaver Options:
                auto slashC = text.find (L"/c");
                auto slashQ = text.find (L"/?");



                Assert::IsTrue (slashC > optionsPos && slashC < screensaverPos,
                                L"/c should be in the Options section");
                Assert::IsTrue (slashQ > optionsPos && slashQ < screensaverPos,
                                L"/? should be in the Options section");
            }




            TEST_METHOD (GetFormattedText_ScreensaverSwitchesInScreensaverSection)
            {
                UsageText usage (L'/');


                const std::wstring & text = usage.GetFormattedText();

                auto screensaverPos = text.find (L"Screensaver Options:");

                // /s, /p, /a should appear after Screensaver Options:
                auto slashS = text.find (L"/s");
                auto slashP = text.find (L"/p");
                auto slashA = text.find (L"/a");



                Assert::IsTrue (slashS > screensaverPos, L"/s should be in the Screensaver Options section");
                Assert::IsTrue (slashP > screensaverPos, L"/p should be in the Screensaver Options section");
                Assert::IsTrue (slashA > screensaverPos, L"/a should be in the Screensaver Options section");
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
