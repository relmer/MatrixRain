#include "Pch_MatrixRainTests.h"
#include "EhmTestHelper.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;





TEST_MODULE_INITIALIZE(ModuleInitialize)
{
    // Setup EHM to not trigger breakpoints during unit tests
    UnitTest::SetupEhmForUnitTests();
}
