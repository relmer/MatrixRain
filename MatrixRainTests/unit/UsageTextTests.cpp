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
                UsageText usage (L"/");


                auto lines = usage.GetFormattedLines();



                Assert::IsFalse (lines.empty(), L"Should have formatted lines");

                // Verify switches use / prefix
                bool foundSlashInstall = false;
                bool foundSlashQ       = false;

                for (const auto & line : lines)
                {
                    if (line.find (L"/install") != std::wstring::npos) foundSlashInstall = true;
                    if (line.find (L"/?") != std::wstring::npos)       foundSlashQ       = true;
                }

                Assert::IsTrue (foundSlashInstall, L"Should contain /install switch");
                Assert::IsTrue (foundSlashQ, L"Should contain /? switch");
            }




            TEST_METHOD (GetSwitchPrefix_SlashPrefix_ReturnsSlash)
            {
                UsageText usage (L"/");



                Assert::AreEqual (std::wstring (L"/"), usage.GetSwitchPrefix(), L"Prefix should be /");
            }




            ////////////////////////////////////////////////////////////
            //  T003: UsageText with - prefix
            ////////////////////////////////////////////////////////////

            TEST_METHOD (GetFormattedLines_DashPrefix_ContainsDashSwitches)
            {
                UsageText usage (L"-");


                auto lines = usage.GetFormattedLines();



                Assert::IsFalse (lines.empty(), L"Should have formatted lines");

                bool foundDashInstall = false;
                bool foundDashQ      = false;

                for (const auto & line : lines)
                {
                    if (line.find (L"-install") != std::wstring::npos) foundDashInstall = true;
                    if (line.find (L"-?") != std::wstring::npos)       foundDashQ       = true;
                }

                Assert::IsTrue (foundDashInstall, L"Should contain -install switch");
                Assert::IsTrue (foundDashQ, L"Should contain -? switch");
            }




            TEST_METHOD (GetSwitchPrefix_DashPrefix_ReturnsDash)
            {
                UsageText usage (L"-");



                Assert::AreEqual (std::wstring (L"-"), usage.GetSwitchPrefix(), L"Prefix should be -");
            }




            ////////////////////////////////////////////////////////////
            //  T004: GetPlainText returns CRLF-joined, no ANSI
            ////////////////////////////////////////////////////////////

            TEST_METHOD (GetPlainText_ReturnsCRLFJoinedLines)
            {
                UsageText usage (L"/");


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
            //  GetOverlayEntries
            ////////////////////////////////////////////////////////////

            TEST_METHOD (GetOverlayEntries_ReturnsNonEmptyVector)
            {
                UsageText usage (L"/");


                auto entries = usage.GetOverlayEntries();



                Assert::IsTrue (entries.size() > 0, L"Should return overlay entries");
            }




            TEST_METHOD (GetOverlayEntries_ContainsVersionLine)
            {
                UsageText usage (L"/");


                auto entries = usage.GetOverlayEntries();



                Assert::IsTrue (entries[0].first.find (L"MatrixRain v") != std::wstring::npos,
                                L"First entry should contain version string");
                Assert::IsTrue (entries[0].second.empty(),
                                L"Version line should be single-column (empty right)");
            }




            TEST_METHOD (GetOverlayEntries_SwitchesAreTwoColumn)
            {
                UsageText usage (L"/");


                auto entries = usage.GetOverlayEntries();
                auto & lastEntry = entries.back();



                Assert::IsFalse (lastEntry.first.empty(),
                                 L"Switch entry should have non-empty left column");
                Assert::IsFalse (lastEntry.second.empty(),
                                 L"Switch entry should have non-empty right column");
            }




            TEST_METHOD (GetOverlayEntries_UsesSwitchPrefix)
            {
                UsageText usage (L"-");


                auto entries = usage.GetOverlayEntries();
                bool foundDashPrefix = false;

                for (const auto & [left, right] : entries)
                {
                    if (!right.empty() && left.find (L'-') != std::wstring::npos)
                    {
                        foundDashPrefix = true;
                        break;
                    }
                }



                Assert::IsTrue (foundDashPrefix,
                                L"Switch entries should use the specified prefix");
            }




            ////////////////////////////////////////////////////////////
            //  T086: Section grouping — GetFormattedText
            ////////////////////////////////////////////////////////////

            TEST_METHOD (GetFormattedText_ContainsOptionsSectionHeader)
            {
                UsageText usage (L"/");


                const std::wstring & text = usage.GetFormattedText();



                Assert::IsTrue (text.find (L"Options:") != std::wstring::npos,
                                L"Should contain 'Options:' section header");
            }




            TEST_METHOD (GetFormattedText_DoesNotContainScreensaverOptions)
            {
                UsageText usage (L"/");


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
                UsageText usage (L"/");


                const std::wstring & text = usage.GetFormattedText();



                Assert::IsTrue (text.find (L"MatrixRain v") != std::wstring::npos,
                                L"Should contain version header");
            }




            TEST_METHOD (GetFormattedText_ContainsCopyrightLine)
            {
                UsageText usage (L"/");


                const std::wstring & text = usage.GetFormattedText();



                Assert::IsTrue (text.find (L"Copyright") != std::wstring::npos,
                                L"Should contain copyright text");
                Assert::IsTrue (text.find (L"Robert Elmer") != std::wstring::npos,
                                L"Should contain author name");
            }




            TEST_METHOD (GetFormattedText_ContainsArchitecture)
            {
                UsageText usage (L"/");


                const std::wstring & text = usage.GetFormattedText();

                // Should contain either x64 or ARM64
                bool hasArch = (text.find (L"x64") != std::wstring::npos) ||
                               (text.find (L"ARM64") != std::wstring::npos);



                Assert::IsTrue (hasArch, L"Should contain architecture string (x64 or ARM64)");
            }




            TEST_METHOD (GetFormattedText_UsesEmDashSeparator)
            {
                UsageText usage (L"/");


                const std::wstring & text = usage.GetFormattedText();



                Assert::IsTrue (text.find (L'\u2014') != std::wstring::npos,
                                L"Should use em dash separator between switch name and description");
            }




            TEST_METHOD (GetFormattedText_DoesNotContainHotkeys)
            {
                UsageText usage (L"/");


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
                UsageText usage (L"/");


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




            ////////////////////////////////////////////////////////////
            //  T030: Help text contains /install switch
            ////////////////////////////////////////////////////////////

            TEST_METHOD (GetFormattedText_ContainsInstallSwitch)
            {
                UsageText usage (L"/");


                const std::wstring & text = usage.GetFormattedText();



                Assert::IsTrue (text.find (L"/install") != std::wstring::npos ||
                                text.find (L"install") != std::wstring::npos,
                                L"Help text should contain install switch");
            }




            ////////////////////////////////////////////////////////////
            //  T031: Help text contains /uninstall switch
            ////////////////////////////////////////////////////////////

            TEST_METHOD (GetFormattedText_ContainsUninstallSwitch)
            {
                UsageText usage (L"/");


                const std::wstring & text = usage.GetFormattedText();



                Assert::IsTrue (text.find (L"/uninstall") != std::wstring::npos ||
                                text.find (L"uninstall") != std::wstring::npos,
                                L"Help text should contain uninstall switch");
            }
    };




}  // namespace MatrixRainTests
