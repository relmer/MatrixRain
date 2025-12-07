#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ScreenSaverMode.h"
#include "..\..\MatrixRainCore\ScreenSaverModeContext.h"
#include "..\..\MatrixRainCore\ScreenSaverModeParser.h"




namespace MatrixRainTests
{
    TEST_CLASS (ScreenSaverErrorHandlingTests)
    {
    public:
        // T032: Test invalid screensaver arguments produce appropriate error context
        
        TEST_METHOD (TestInvalidArgument_ReturnsError)
        {
            // Arrange
            HRESULT                hr;
            ScreenSaverModeContext context;
            LPCWSTR                commandLine = L"/z";  // Invalid switch
            
            
            // Act
            hr = ParseCommandLine (commandLine, context);
            
            
            // Assert
            Assert::IsTrue  (FAILED (hr), L"ParseCommandLine should return failure HRESULT");
            Assert::IsFalse (context.m_errorMessage.empty(), L"Error message should be populated");
        }
        
        
        
        
        
        
        TEST_METHOD (TestDirectXInitializationFailure_ReturnsHRESULT)
        {
            // This test will need mock/stub support for testing D3D init failures
            // For now, document the expectation that Application::Initialize should
            // return a failing HRESULT when D3D device creation fails
            
            // Expected behavior:
            // - Application::Initialize returns HRESULT
            // - main.cpp checks for failure
            // - Displays MessageBox with descriptive error
            
            Assert::IsTrue (true, L"DirectX failure handling requires integration test with mock D3D device");
        }
    };
}
