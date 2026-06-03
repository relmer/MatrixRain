#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\DeviceLost.h"




namespace MatrixRainTests
{


    TEST_CLASS (DeviceLostTests)
    {
        public:

            TEST_METHOD (IsDeviceLost_DeviceRemoved_ReturnsTrue)
            {
                Assert::IsTrue (IsDeviceLost (DXGI_ERROR_DEVICE_REMOVED));
            }




            TEST_METHOD (IsDeviceLost_DeviceReset_ReturnsTrue)
            {
                Assert::IsTrue (IsDeviceLost (DXGI_ERROR_DEVICE_RESET));
            }




            TEST_METHOD (IsDeviceLost_DeviceHung_ReturnsTrue)
            {
                Assert::IsTrue (IsDeviceLost (DXGI_ERROR_DEVICE_HUNG));
            }




            TEST_METHOD (IsDeviceLost_DriverInternalError_ReturnsTrue)
            {
                Assert::IsTrue (IsDeviceLost (DXGI_ERROR_DRIVER_INTERNAL_ERROR));
            }




            TEST_METHOD (IsDeviceLost_Success_ReturnsFalse)
            {
                Assert::IsFalse (IsDeviceLost (S_OK));
            }




            TEST_METHOD (IsDeviceLost_GenericFailure_ReturnsFalse)
            {
                Assert::IsFalse (IsDeviceLost (E_FAIL));
            }




            TEST_METHOD (IsDeviceLost_InvalidArg_ReturnsFalse)
            {
                Assert::IsFalse (IsDeviceLost (E_INVALIDARG));
            }




            TEST_METHOD (IsDeviceLost_Occluded_ReturnsFalse)
            {
                Assert::IsFalse (IsDeviceLost (DXGI_STATUS_OCCLUDED));
            }
    };


}
