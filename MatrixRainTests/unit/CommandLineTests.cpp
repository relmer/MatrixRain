#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ScreenSaverMode.h"
#include "..\..\MatrixRainCore\ScreenSaverModeContext.h"
#include "..\..\MatrixRainCore\CommandLine.h"





namespace MatrixRainTests
{




    TEST_CLASS (CommandLineTests)
    {
        public:
            // T010: Verify /s launches screensaver full-screen mode
            TEST_METHOD (ParseCommandLine_SlashS_ReturnsScreenSaverFullMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"/s", context);



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


                hr = CommandLine().Parse (L"/s", contextLower);
                hr = CommandLine().Parse (L"/S", contextUpper);



                Assert::AreEqual (static_cast<int>(contextLower.m_mode), static_cast<int>(contextUpper.m_mode), L"mode should match regardless of case");
            }




            // T010: Verify /p <HWND> launches preview mode
            TEST_METHOD (ParseCommandLine_SlashP_ReturnsPreviewMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;
                HWND                   testHwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(0x12345678));
                std::wstring           cmdLine  = std::format (L"/p {}", reinterpret_cast<uintptr_t>(testHwnd));


                hr = CommandLine().Parse (cmdLine.c_str(), context);



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


                hr = CommandLine().Parse (L"/c", context);



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


                hr = CommandLine().Parse (cmdLine.c_str(), context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::SettingsDialog), static_cast<int>(context.m_mode), L"mode should be SettingsDialog");
                Assert::IsTrue  (testHwnd == context.m_previewParentHwnd, L"parent HWND should be parsed");
            }




            // T010: Verify /a launches password-change-unsupported mode
            TEST_METHOD (ParseCommandLine_SlashA_ReturnsPasswordChangeUnsupported)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"/a", context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::PasswordChangeUnsupported), static_cast<int>(context.m_mode), L"mode should be PasswordChangeUnsupported");
            }




            // T010: Verify no arguments defaults to Normal mode
            TEST_METHOD (ParseCommandLine_NoArguments_ReturnsNormalMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"", context);



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


                hr = CommandLine().Parse (nullptr, context);
                Assert::IsTrue (SUCCEEDED (hr), L"ParseCommandLine should return success HRESULT");
                Assert::IsTrue (context.m_errorMessage.empty(), L"Error message should not be populated");

                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"mode should be Normal");
            }




            // T010: Verify whitespace-only command line defaults to Normal
            TEST_METHOD (ParseCommandLine_WhitespaceOnly_ReturnsNormalMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"   \t  ", context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"whitespace-only should default to Normal");
            }




            // T010: Verify invalid /p without HWND is handled gracefully
            TEST_METHOD (ParseCommandLine_SlashP_MissingHwnd_DefaultsToNormalMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;

                hr = CommandLine().Parse (L"/p", context);
                hr = CommandLine().Parse (L"/p", context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"invalid /p should default to Normal");
            }




            // T010: Verify invalid /p with non-numeric HWND is handled
            TEST_METHOD (ParseCommandLine_SlashP_InvalidHwnd_DefaultsToNormalMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"/p invalid", context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"invalid HWND should default to Normal");
            }




            // T010: Verify unknown arguments default to Normal mode
            TEST_METHOD (ParseCommandLine_UnknownArgument_DefaultsToNormalMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"/x", context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"unknown argument should default to Normal");
            }




            ////////////////////////////////////////////////////////////
            //  T010: /? and -? return HelpRequested mode
            ////////////////////////////////////////////////////////////

            TEST_METHOD (ParseCommandLine_SlashQuestion_ReturnsHelpRequested)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"/?", context);



                Assert::IsTrue (SUCCEEDED (hr), L"ParseCommandLine should succeed for /?");
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::HelpRequested), static_cast<int>(context.m_mode), L"mode should be HelpRequested");
                Assert::AreEqual (L'/', context.m_switchPrefix, L"switch prefix should be /");
            }




            TEST_METHOD (ParseCommandLine_DashQuestion_ReturnsHelpRequested)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"-?", context);



                Assert::IsTrue (SUCCEEDED (hr), L"ParseCommandLine should succeed for -?");
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::HelpRequested), static_cast<int>(context.m_mode), L"mode should be HelpRequested");
                Assert::AreEqual (L'-', context.m_switchPrefix, L"switch prefix should be -");
            }




            ////////////////////////////////////////////////////////////
            //  T010: All preexisting switches accept - prefix
            ////////////////////////////////////////////////////////////

            TEST_METHOD (ParseCommandLine_DashS_ReturnsScreenSaverFullMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"-s", context);



                Assert::IsTrue (SUCCEEDED (hr), L"ParseCommandLine should succeed for -s");
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::ScreenSaverFull), static_cast<int>(context.m_mode), L"-s should give ScreenSaverFull");
            }




            TEST_METHOD (ParseCommandLine_DashC_ReturnsSettingsDialogMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"-c", context);



                Assert::IsTrue (SUCCEEDED (hr), L"ParseCommandLine should succeed for -c");
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::SettingsDialog), static_cast<int>(context.m_mode), L"-c should give SettingsDialog");
            }




            TEST_METHOD (ParseCommandLine_DashA_ReturnsPasswordChangeUnsupported)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"-a", context);



                Assert::IsTrue (SUCCEEDED (hr), L"ParseCommandLine should succeed for -a");
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::PasswordChangeUnsupported), static_cast<int>(context.m_mode), L"-a should give PasswordChangeUnsupported");
            }




            TEST_METHOD (ParseCommandLine_DashS_CaseInsensitive)
            {
                HRESULT                hr;
                ScreenSaverModeContext contextLower;
                ScreenSaverModeContext contextUpper;


                hr = CommandLine().Parse (L"-s", contextLower);
                hr = CommandLine().Parse (L"-S", contextUpper);



                Assert::AreEqual (static_cast<int>(contextLower.m_mode), static_cast<int>(contextUpper.m_mode), L"-s and -S should give same mode");
            }




            ////////////////////////////////////////////////////////////
            //  T007-T013: Multi-character switch tests (install/uninstall)
            ////////////////////////////////////////////////////////////

            TEST_METHOD (ParseCommandLine_SlashInstall_ReturnsInstallMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"/install", context);



                Assert::IsTrue (SUCCEEDED (hr), L"ParseCommandLine should succeed for /install");
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Install), static_cast<int>(context.m_mode), L"mode should be Install");
            }




            TEST_METHOD (ParseCommandLine_SlashUninstall_ReturnsUninstallMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"/uninstall", context);



                Assert::IsTrue (SUCCEEDED (hr), L"ParseCommandLine should succeed for /uninstall");
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Uninstall), static_cast<int>(context.m_mode), L"mode should be Uninstall");
            }




            TEST_METHOD (ParseCommandLine_DoubleDashInstall_ReturnsInstallMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"--install", context);



                Assert::IsTrue (SUCCEEDED (hr), L"ParseCommandLine should succeed for --install");
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Install), static_cast<int>(context.m_mode), L"mode should be Install");
            }




            TEST_METHOD (ParseCommandLine_DoubleDashUninstall_ReturnsUninstallMode)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"--uninstall", context);



                Assert::IsTrue (SUCCEEDED (hr), L"ParseCommandLine should succeed for --uninstall");
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Uninstall), static_cast<int>(context.m_mode), L"mode should be Uninstall");
            }




            TEST_METHOD (ParseCommandLine_SlashInstall_CaseInsensitive)
            {
                HRESULT                hr;
                ScreenSaverModeContext contextLower;
                ScreenSaverModeContext contextUpper;
                ScreenSaverModeContext contextMixed;


                hr = CommandLine().Parse (L"/install", contextLower);
                hr = CommandLine().Parse (L"/INSTALL", contextUpper);
                hr = CommandLine().Parse (L"/Install", contextMixed);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Install), static_cast<int>(contextLower.m_mode), L"/install should be Install");
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Install), static_cast<int>(contextUpper.m_mode), L"/INSTALL should be Install");
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Install), static_cast<int>(contextMixed.m_mode), L"/Install should be Install");
            }




            TEST_METHOD (ParseCommandLine_SlashUninstall_CaseInsensitive)
            {
                HRESULT                hr;
                ScreenSaverModeContext contextLower;
                ScreenSaverModeContext contextUpper;
                ScreenSaverModeContext contextMixed;


                hr = CommandLine().Parse (L"/uninstall", contextLower);
                hr = CommandLine().Parse (L"/UNINSTALL", contextUpper);
                hr = CommandLine().Parse (L"/Uninstall", contextMixed);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Uninstall), static_cast<int>(contextLower.m_mode), L"/uninstall should be Uninstall");
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Uninstall), static_cast<int>(contextUpper.m_mode), L"/UNINSTALL should be Uninstall");
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Uninstall), static_cast<int>(contextMixed.m_mode), L"/Uninstall should be Uninstall");
            }




            TEST_METHOD (ParseCommandLine_ExistingSingleCharSwitches_StillWork)
            {
                HRESULT                hr;
                ScreenSaverModeContext ctxS, ctxC, ctxA, ctxHelp;
                ScreenSaverModeContext ctxP;
                std::wstring           pCmd = std::format (L"/p {}", static_cast<uintptr_t>(0x12345678));


                hr = CommandLine().Parse (L"/s", ctxS);
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::ScreenSaverFull), static_cast<int>(ctxS.m_mode), L"/s regression");

                hr = CommandLine().Parse (L"/c", ctxC);
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::SettingsDialog), static_cast<int>(ctxC.m_mode), L"/c regression");

                hr = CommandLine().Parse (pCmd.c_str(), ctxP);
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::ScreenSaverPreview), static_cast<int>(ctxP.m_mode), L"/p regression");

                hr = CommandLine().Parse (L"/a", ctxA);
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::PasswordChangeUnsupported), static_cast<int>(ctxA.m_mode), L"/a regression");

                hr = CommandLine().Parse (L"/?", ctxHelp);
                Assert::AreEqual (static_cast<int>(ScreenSaverMode::HelpRequested), static_cast<int>(ctxHelp.m_mode), L"/? regression");
            }




            TEST_METHOD (ParseCommandLine_SingleDashInstall_RejectsLongSwitch)
            {
                HRESULT                hr;
                ScreenSaverModeContext context;


                hr = CommandLine().Parse (L"-install", context);



                Assert::AreEqual (static_cast<int>(ScreenSaverMode::Normal), static_cast<int>(context.m_mode), L"-install should not be recognized");
            }
    };
}  // namespace MatrixRainTests
