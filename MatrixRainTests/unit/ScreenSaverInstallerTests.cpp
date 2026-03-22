#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ScreenSaverInstaller.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;





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
    };
}
