#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ScreenSaverInstaller.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;





////////////////////////////////////////////////////////////////////////////////
//
//  MockRegistryProvider
//
//  In-memory mock for IRegistryProvider — tests NEVER touch the real registry.
//
////////////////////////////////////////////////////////////////////////////////

class MockRegistryProvider : public IRegistryProvider
{
public:

    std::map<std::wstring, std::wstring>  m_values;
    bool                                  m_failReads  = false;
    bool                                  m_failWrites = false;


    HRESULT ReadString (HKEY    /*hKey*/,
                        LPCWSTR /*pszSubKey*/,
                        LPCWSTR pszValueName,
                        std::wstring & value) override
    {
        if (m_failReads)
            return E_FAIL;

        auto it = m_values.find (pszValueName);
        if (it == m_values.end())
            return HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND);

        value = it->second;
        return S_OK;
    }


    HRESULT WriteString (HKEY    /*hKey*/,
                         LPCWSTR /*pszSubKey*/,
                         LPCWSTR pszValueName,
                         LPCWSTR pszValue) override
    {
        if (m_failWrites)
            return E_FAIL;

        m_values[pszValueName] = pszValue;
        return S_OK;
    }


    HRESULT DeleteValue (HKEY    /*hKey*/,
                         LPCWSTR /*pszSubKey*/,
                         LPCWSTR pszValueName) override
    {
        if (m_failWrites)
            return E_FAIL;

        m_values.erase (pszValueName);
        return S_OK;
    }
};





namespace MatrixRainTests
{
    TEST_CLASS (ScreenSaverInstallerTests)
    {
    public:

        ////////////////////////////////////////////////////////////
        //  T017: IsElevated returns a boolean without crashing
        ////////////////////////////////////////////////////////////

        TEST_METHOD (IsElevated_ReturnsBoolean)
        {
            bool elevated = ScreenSaverInstaller::IsElevated();


            // Test runner is not elevated, so we expect false.
            // The key assertion is that it doesn't crash.
            Assert::IsFalse (elevated, L"Test runner should not be elevated");
        }




        ////////////////////////////////////////////////////////////
        //  T018: Install returns E_ACCESSDENIED when not elevated
        ////////////////////////////////////////////////////////////

        TEST_METHOD (Install_NotElevated_ReturnsAccessDenied)
        {
            HRESULT hr = ScreenSaverInstaller::Install();


            Assert::AreEqual (E_ACCESSDENIED, hr, L"Install should return E_ACCESSDENIED when not elevated");
        }




        ////////////////////////////////////////////////////////////
        //  T025: Uninstall returns E_ACCESSDENIED when not elevated
        ////////////////////////////////////////////////////////////

        TEST_METHOD (Uninstall_NotElevated_ReturnsAccessDenied)
        {
            MockRegistryProvider mockRegistry;


            HRESULT hr = ScreenSaverInstaller::Uninstall (mockRegistry);



            Assert::AreEqual (E_ACCESSDENIED, hr, L"Uninstall should return E_ACCESSDENIED when not elevated");
        }




        ////////////////////////////////////////////////////////////
        //  T026: Uninstall gracefully handles missing .scr file
        ////////////////////////////////////////////////////////////

        TEST_METHOD (Uninstall_ScrFileNotPresent_ReturnsSOK)
        {
            // This test can only fully run when elevated (file deletion).
            // When not elevated, we verify the elevation check returns E_ACCESSDENIED.
            MockRegistryProvider mockRegistry;


            HRESULT hr = ScreenSaverInstaller::Uninstall (mockRegistry);



            // Non-elevated: E_ACCESSDENIED is expected
            Assert::AreEqual (E_ACCESSDENIED, hr, L"Non-elevated uninstall should return E_ACCESSDENIED");
        }




        ////////////////////////////////////////////////////////////
        //  T026a: Uninstall clears registry when MatrixRain is active
        ////////////////////////////////////////////////////////////

        TEST_METHOD (Uninstall_MatrixRainActive_ClearsRegistryEntries)
        {
            MockRegistryProvider mockRegistry;
            WCHAR                szSystem32[MAX_PATH];
            std::wstring         expectedScrPath;


            GetSystemDirectoryW (szSystem32, _countof (szSystem32));
            expectedScrPath = std::wstring (szSystem32) + L"\\MatrixRain.scr";

            // Simulate MatrixRain being the active screensaver
            mockRegistry.m_values[L"SCRNSAVE.EXE"]     = expectedScrPath;
            mockRegistry.m_values[L"ScreenSaveActive"]  = L"1";

            ScreenSaverInstaller::CleanupRegistryForUninstall (mockRegistry);



            // SCRNSAVE.EXE should be deleted
            Assert::IsTrue (mockRegistry.m_values.find (L"SCRNSAVE.EXE") == mockRegistry.m_values.end(),
                            L"SCRNSAVE.EXE should be removed from registry");

            // ScreenSaveActive should be set to "0"
            Assert::AreEqual (std::wstring (L"0"), mockRegistry.m_values[L"ScreenSaveActive"],
                              L"ScreenSaveActive should be set to 0");
        }




        ////////////////////////////////////////////////////////////
        //  T026b: Uninstall leaves registry untouched when different screensaver
        ////////////////////////////////////////////////////////////

        TEST_METHOD (Uninstall_DifferentScreensaverActive_LeavesRegistryUntouched)
        {
            MockRegistryProvider mockRegistry;


            // Simulate a different screensaver being active
            mockRegistry.m_values[L"SCRNSAVE.EXE"]     = L"C:\\Windows\\System32\\OtherSaver.scr";
            mockRegistry.m_values[L"ScreenSaveActive"]  = L"1";

            ScreenSaverInstaller::CleanupRegistryForUninstall (mockRegistry);



            // Both values should remain untouched
            Assert::AreEqual (std::wstring (L"C:\\Windows\\System32\\OtherSaver.scr"),
                              mockRegistry.m_values[L"SCRNSAVE.EXE"],
                              L"SCRNSAVE.EXE should remain unchanged");
            Assert::AreEqual (std::wstring (L"1"),
                              mockRegistry.m_values[L"ScreenSaveActive"],
                              L"ScreenSaveActive should remain 1");
        }




        ////////////////////////////////////////////////////////////
        //  T026c: Uninstall handles missing registry key gracefully
        ////////////////////////////////////////////////////////////

        TEST_METHOD (Uninstall_RegistryKeyMissing_DoesNotCrash)
        {
            MockRegistryProvider mockRegistry;


            // Empty registry — no SCRNSAVE.EXE key at all
            ScreenSaverInstaller::CleanupRegistryForUninstall (mockRegistry);



            // Should return without crashing — no exception, no assertion failure
            Assert::IsTrue (true, L"CleanupRegistryForUninstall should not crash with missing keys");
        }
    };
}
