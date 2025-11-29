#include "pch.h"
#include "EhmTestHelper.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    void UnitTestEhmBreakpoint()
    {
        // Call the CppUnitTestFramework's Assert::Fail() method instead of __debugbreak()
        // This will properly register the failure with the unit testing framework
        Assert::Fail(L"EHM assertion failure detected in unit test");
    }

    void SetupEhmForUnitTests()
    {
        // Set the custom breakpoint function for EHM to use during unit tests
        SetBreakpointFunction(UnitTestEhmBreakpoint);
    }
}
