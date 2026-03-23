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





////////////////////////////////////////////////////////////////////////////////
//
//  MockFileSystemProvider
//
//  In-memory mock for IFileSystemProvider — tests NEVER touch the real FS.
//
////////////////////////////////////////////////////////////////////////////////

class MockFileSystemProvider : public IFileSystemProvider
{
public:

    std::wstring  m_systemDirectory    = L"C:\\Windows\\System32";
    DWORD         m_fileAttributes     = INVALID_FILE_ATTRIBUTES;
    DWORD         m_lastError          = ERROR_FILE_NOT_FOUND;
    bool          m_deleteFileSuccess  = true;
    std::wstring  m_longPathResult;


    DWORD GetSystemDirectory (LPWSTR lpBuffer, UINT uSize) override
    {
        if (m_systemDirectory.size() >= uSize)
            return 0;

        wcscpy_s (lpBuffer, uSize, m_systemDirectory.c_str());
        return static_cast<DWORD> (m_systemDirectory.size());
    }


    DWORD GetFileAttributes (LPCWSTR /*lpFileName*/) override
    {
        return m_fileAttributes;
    }


    DWORD GetLastError () override
    {
        return m_lastError;
    }


    BOOL DeleteFile (LPCWSTR /*lpFileName*/) override
    {
        return m_deleteFileSuccess ? TRUE : FALSE;
    }


    DWORD GetLongPathName (LPCWSTR lpszShortPath,
                           LPWSTR  lpszLongPath,
                           DWORD   cchBuffer) override
    {
        // If no override set, return the input unchanged
        const std::wstring & result = m_longPathResult.empty()
            ? std::wstring (lpszShortPath)
            : m_longPathResult;

        if (result.size() >= cchBuffer)
            return 0;

        wcscpy_s (lpszLongPath, cchBuffer, result.c_str());
        return static_cast<DWORD> (result.size());
    }
};





namespace MatrixRainTests
{
    TEST_CLASS (ScreenSaverInstallerTests)
    {
    public:

        TEST_METHOD_CLEANUP (ResetElevationMock)
        {
            ScreenSaverInstaller::s_pfnIsElevated = &ScreenSaverInstaller::IsElevated;
        }




        ////////////////////////////////////////////////////////////
        //  T017: IsElevated returns a boolean without crashing
        ////////////////////////////////////////////////////////////

        TEST_METHOD (IsElevated_ReturnsBoolean)
        {
            // Verify IsElevated() runs without crashing; the result depends on
            // the test runner's security context and cannot be asserted.
            bool elevated = ScreenSaverInstaller::IsElevated();


            UNREFERENCED_PARAMETER (elevated);
        }




        ////////////////////////////////////////////////////////////
        //  T018: Install returns E_ACCESSDENIED when not elevated
        ////////////////////////////////////////////////////////////

        TEST_METHOD (Install_NotElevated_ReturnsAccessDenied)
        {
            ScreenSaverInstaller::s_pfnIsElevated = []() { return false; };


            HRESULT hr = ScreenSaverInstaller::Install();



            Assert::AreEqual (E_ACCESSDENIED, hr, L"Install should return E_ACCESSDENIED when not elevated");
        }




        ////////////////////////////////////////////////////////////
        //  T025: Uninstall returns E_ACCESSDENIED when not elevated
        ////////////////////////////////////////////////////////////

        TEST_METHOD (Uninstall_NotElevated_ReturnsAccessDenied)
        {
            ScreenSaverInstaller::s_pfnIsElevated = []() { return false; };

            MockRegistryProvider     mockRegistry;
            MockFileSystemProvider   mockFS;


            HRESULT hr = ScreenSaverInstaller::Uninstall (mockRegistry, mockFS);



            Assert::AreEqual (E_ACCESSDENIED, hr, L"Uninstall should return E_ACCESSDENIED when not elevated");
        }




        ////////////////////////////////////////////////////////////
        //  T026: Uninstall gracefully handles missing .scr file
        ////////////////////////////////////////////////////////////

        TEST_METHOD (Uninstall_ScrFileNotPresent_ReturnsSOK)
        {
            ScreenSaverInstaller::s_pfnIsElevated = []() { return true; };

            MockRegistryProvider     mockRegistry;
            MockFileSystemProvider   mockFS;

            mockFS.m_fileAttributes = INVALID_FILE_ATTRIBUTES;
            mockFS.m_lastError      = ERROR_FILE_NOT_FOUND;


            HRESULT hr = ScreenSaverInstaller::Uninstall (mockRegistry, mockFS);



            Assert::AreEqual (S_FALSE, hr, L"Uninstall should return S_FALSE when .scr file not present");
        }




        ////////////////////////////////////////////////////////////
        //  T026a: Uninstall clears registry when MatrixRain is active
        ////////////////////////////////////////////////////////////

        TEST_METHOD (Uninstall_MatrixRainActive_ClearsRegistryEntries)
        {
            MockRegistryProvider     mockRegistry;
            MockFileSystemProvider   mockFS;
            std::wstring             expectedScrPath;


            expectedScrPath = mockFS.m_systemDirectory + L"\\MatrixRain.scr";

            // Simulate MatrixRain being the active screensaver
            mockRegistry.m_values[L"SCRNSAVE.EXE"]     = expectedScrPath;
            mockRegistry.m_values[L"ScreenSaveActive"]  = L"1";

            ScreenSaverInstaller::CleanupRegistryForUninstall (mockRegistry, mockFS);



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
            MockRegistryProvider     mockRegistry;
            MockFileSystemProvider   mockFS;


            // Simulate a different screensaver being active
            mockRegistry.m_values[L"SCRNSAVE.EXE"]     = L"C:\\Windows\\System32\\OtherSaver.scr";
            mockRegistry.m_values[L"ScreenSaveActive"]  = L"1";

            ScreenSaverInstaller::CleanupRegistryForUninstall (mockRegistry, mockFS);



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
            MockRegistryProvider     mockRegistry;
            MockFileSystemProvider   mockFS;


            // Empty registry — no SCRNSAVE.EXE key at all
            ScreenSaverInstaller::CleanupRegistryForUninstall (mockRegistry, mockFS);



            // Should return without crashing — no exception, no assertion failure
            Assert::IsTrue (true, L"CleanupRegistryForUninstall should not crash with missing keys");
        }
    };
}
