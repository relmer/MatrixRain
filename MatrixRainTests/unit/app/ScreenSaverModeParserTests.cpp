#include "pch.h"




#include "matrixrain/ScreenSaverMode.h"
#include "matrixrain/ScreenSaverModeContext.h"
#include "matrixrain/ScreenSaverModeParser.h"




namespace MatrixRainTests
{




    TEST_CLASS (ScreenSaverModeParserTests)
    {
        public:
            // T010: Verify /s launches screensaver full-screen mode
            TEST_METHOD (ParseCommandLine_SlashS_ReturnsScreenSaverFullMode)
            {
                ScreenSaverModeContext context = ParseCommandLine (L"/s");



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::ScreenSaverFull), static_cast<int>(context.m_mode), L"mode should be ScreenSaverFull");
                Assert::IsFalse (context.m_enableHotkeys, L"hotkeys should be disabled");
                Assert::IsTrue  (context.m_hideCursor, L"cursor should be hidden");
                Assert::IsTrue  (context.m_exitOnInput, L"should exit on input");
                Assert::IsTrue  (context.m_suppressDebug, L"debug overlays should be suppressed");
                Assert::IsNull  (context.m_previewParentHwnd, L"preview HWND should be null");
            }




            // T010: Verify /S (uppercase) works identically to /s
            TEST_METHOD (ParseCommandLine_SlashS_CaseInsensitive)
            {
                ScreenSaverModeContext contextLower = ParseCommandLine (L"/s");
                ScreenSaverModeContext contextUpper = ParseCommandLine (L"/S");



                Assert::AreEqual (static_cast<int>(contextLower.m_mode), static_cast<int>(contextUpper.m_mode), L"mode should match regardless of case");
            }




            // T010: Verify /p <HWND> launches preview mode
            TEST_METHOD (ParseCommandLine_SlashP_ReturnsPreviewMode)
            {
                HWND         testHwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(0x12345678));
                std::wstring cmdLine  = std::format (L"/p {}", reinterpret_cast<uintptr_t>(testHwnd));

                ScreenSaverModeContext context = ParseCommandLine (cmdLine.c_str());



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::ScreenSaverPreview), static_cast<int>(context.m_mode), L"mode should be ScreenSaverPreview");
                Assert::IsTrue (testHwnd == context.m_previewParentHwnd, L"preview HWND should match");
                Assert::IsFalse (context.m_enableHotkeys, L"hotkeys should be disabled");
                Assert::IsFalse (context.m_hideCursor, L"cursor should remain visible in preview");
                Assert::IsFalse (context.m_exitOnInput, L"should not exit on input in preview");
                Assert::IsTrue  (context.m_suppressDebug, L"debug overlays suppressed in preview");
            }




            // T010: Verify /c launches settings dialog mode
            TEST_METHOD (ParseCommandLine_SlashC_ReturnsSettingsDialogMode)
            {
                ScreenSaverModeContext context = ParseCommandLine (L"/c");



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::SettingsDialog), static_cast<int>(context.m_mode), L"mode should be SettingsDialog");
                Assert::IsFalse (context.m_enableHotkeys, L"hotkeys not relevant for config dialog");
                Assert::IsFalse (context.m_hideCursor, L"cursor visible in dialog");
                Assert::IsFalse (context.m_exitOnInput, L"dialog manages its own lifetime");
            }




            // T010: Verify /c with optional HWND parameter
            TEST_METHOD (ParseCommandLine_SlashC_WithHwnd_ParsesParentWindow)
            {
                HWND         testHwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(0xABCDEF00));
                std::wstring cmdLine  = std::format (L"/c:{}", reinterpret_cast<uintptr_t>(testHwnd));

                ScreenSaverModeContext context = ParseCommandLine (cmdLine.c_str());



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::SettingsDialog), static_cast<int>(context.m_mode), L"mode should be SettingsDialog");
                Assert::IsTrue  (testHwnd == context.m_previewParentHwnd, L"parent HWND should be parsed");
            }




            // T010: Verify /a launches password-change-unsupported mode
            TEST_METHOD (ParseCommandLine_SlashA_ReturnsPasswordChangeUnsupported)
            {
                ScreenSaverModeContext context = ParseCommandLine (L"/a");



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::PasswordChangeUnsupported), static_cast<int>(context.m_mode), L"mode should be PasswordChangeUnsupported");
            }




            // T010: Verify no arguments defaults to Normal mode
            TEST_METHOD (ParseCommandLine_NoArguments_ReturnsNormalMode)
            {
                ScreenSaverModeContext context = ParseCommandLine (L"");



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"mode should be Normal");
                Assert::IsTrue  (context.m_enableHotkeys, L"hotkeys enabled in normal mode");
                Assert::IsFalse (context.m_hideCursor, L"cursor visible in normal mode");
                Assert::IsFalse (context.m_exitOnInput, L"normal mode does not exit on input");
                Assert::IsFalse (context.m_suppressDebug, L"debug overlays allowed in normal mode");
            }




            // T010: Verify nullptr command line defaults to Normal mode
            TEST_METHOD (ParseCommandLine_Nullptr_ReturnsNormalMode)
            {
                ScreenSaverModeContext context = ParseCommandLine (nullptr);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"mode should be Normal");
            }




            // T010: Verify whitespace-only command line defaults to Normal
            TEST_METHOD (ParseCommandLine_WhitespaceOnly_ReturnsNormalMode)
            {
                ScreenSaverModeContext context = ParseCommandLine (L"   \t  ");



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"whitespace-only should default to Normal");
            }




            // T010: Verify invalid /p without HWND is handled gracefully
            TEST_METHOD (ParseCommandLine_SlashP_MissingHwnd_DefaultsToNormalMode)
            {
                ScreenSaverModeContext context = ParseCommandLine (L"/p");



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"invalid /p should default to Normal");
            }




            // T010: Verify invalid /p with non-numeric HWND is handled
            TEST_METHOD (ParseCommandLine_SlashP_InvalidHwnd_DefaultsToNormalMode)
            {
                ScreenSaverModeContext context = ParseCommandLine (L"/p invalid");



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"invalid HWND should default to Normal");
            }




            // T010: Verify unknown arguments default to Normal mode
            TEST_METHOD (ParseCommandLine_UnknownArgument_DefaultsToNormalMode)
            {
                ScreenSaverModeContext context = ParseCommandLine (L"/x");



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"unknown argument should default to Normal");
            }
    };
}  // namespace MatrixRainTests
