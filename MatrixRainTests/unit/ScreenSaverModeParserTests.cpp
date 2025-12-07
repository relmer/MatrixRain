#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ScreenSaverMode.h"
#include "..\..\MatrixRainCore\ScreenSaverModeContext.h"
#include "..\..\MatrixRainCore\ScreenSaverModeParser.h"





namespace MatrixRainTests
{




    TEST_CLASS (ScreenSaverModeParserTests)
    {
        public:
            // T010: Verify /s launches screensaver full-screen mode
            TEST_METHOD (ParseCommandLine_SlashS_ReturnsScreenSaverFullMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = ParseCommandLine (L"/s", context);



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
                HRESULT                hr;
                ScreenSaverModeContext contextLower;
                ScreenSaverModeContext contextUpper;


                hr = ParseCommandLine (L"/s", contextLower);
                hr = ParseCommandLine (L"/S", contextUpper);



                Assert::AreEqual (static_cast<int>(contextLower.m_mode), static_cast<int>(contextUpper.m_mode), L"mode should match regardless of case");
            }




            // T010: Verify /p <HWND> launches preview mode
            TEST_METHOD (ParseCommandLine_SlashP_ReturnsPreviewMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;
                HWND                   testHwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(0x12345678));
                std::wstring           cmdLine  = std::format (L"/p {}", reinterpret_cast<uintptr_t>(testHwnd));


                hr = ParseCommandLine (cmdLine.c_str(), context);



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
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = ParseCommandLine (L"/c", context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::SettingsDialog), static_cast<int>(context.m_mode), L"mode should be SettingsDialog");
                Assert::IsFalse (context.m_enableHotkeys, L"hotkeys not relevant for config dialog");
                Assert::IsFalse (context.m_hideCursor, L"cursor visible in dialog");
                Assert::IsFalse (context.m_exitOnInput, L"dialog manages its own lifetime");
            }




            // T010: Verify /c with optional HWND parameter
            TEST_METHOD (ParseCommandLine_SlashC_WithHwnd_ParsesParentWindow)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;
                HWND                   testHwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(0xABCDEF00));
                std::wstring           cmdLine  = std::format (L"/c:{}", reinterpret_cast<uintptr_t>(testHwnd));


                hr = ParseCommandLine (cmdLine.c_str(), context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::SettingsDialog), static_cast<int>(context.m_mode), L"mode should be SettingsDialog");
                Assert::IsTrue  (testHwnd == context.m_previewParentHwnd, L"parent HWND should be parsed");
            }




            // T010: Verify /a launches password-change-unsupported mode
            TEST_METHOD (ParseCommandLine_SlashA_ReturnsPasswordChangeUnsupported)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = ParseCommandLine (L"/a", context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::PasswordChangeUnsupported), static_cast<int>(context.m_mode), L"mode should be PasswordChangeUnsupported");
            }




            // T010: Verify no arguments defaults to Normal mode
            TEST_METHOD (ParseCommandLine_NoArguments_ReturnsNormalMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = ParseCommandLine (L"", context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"mode should be Normal");
                Assert::IsTrue  (context.m_enableHotkeys, L"hotkeys enabled in normal mode");
                Assert::IsFalse (context.m_hideCursor, L"cursor visible in normal mode");
                Assert::IsFalse (context.m_exitOnInput, L"normal mode does not exit on input");
                Assert::IsFalse (context.m_suppressDebug, L"debug overlays allowed in normal mode");
            }




            // T010: Verify nullptr command line defaults to Normal mode
            TEST_METHOD (ParseCommandLine_Nullptr_ReturnsNormalMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = ParseCommandLine (nullptr, context);
                Assert::IsTrue (SUCCEEDED (hr), L"ParseCommandLine should return success HRESULT");
                Assert::IsTrue (context.m_errorMessage.empty(), L"Error message should not be populated");

                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"mode should be Normal");
            }




            // T010: Verify whitespace-only command line defaults to Normal
            TEST_METHOD (ParseCommandLine_WhitespaceOnly_ReturnsNormalMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = ParseCommandLine (L"   \t  ", context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"whitespace-only should default to Normal");
            }




            // T010: Verify invalid /p without HWND is handled gracefully
            TEST_METHOD (ParseCommandLine_SlashP_MissingHwnd_DefaultsToNormalMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;

                hr = ParseCommandLine (L"/p", context);
                hr = ParseCommandLine (L"/p", context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"invalid /p should default to Normal");
            }




            // T010: Verify invalid /p with non-numeric HWND is handled
            TEST_METHOD (ParseCommandLine_SlashP_InvalidHwnd_DefaultsToNormalMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = ParseCommandLine (L"/p invalid", context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"invalid HWND should default to Normal");
            }




            // T010: Verify unknown arguments default to Normal mode
            TEST_METHOD (ParseCommandLine_UnknownArgument_DefaultsToNormalMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = ParseCommandLine (L"/x", context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"unknown argument should default to Normal");
            }
    };
}  // namespace MatrixRainTests
