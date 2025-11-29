#pragma once





namespace UnitTest
{
    /// <summary>
    /// Sets up EHM to use the unit test framework's assertion mechanism
    /// instead of __debugbreak() so that EHM failures are properly logged
    /// as unit test failures.
    /// </summary>
    void SetupEhmForUnitTests();

    /// <summary>
    /// Custom breakpoint function that calls the CppUnitTestFramework's
    /// Assert::Fail() method instead of __debugbreak().
    /// </summary>
    void UnitTestEhmBreakpoint();
}
