#pragma once




////////////////////////////////////////////////////////////////////////////////
//
//  IsDeviceLost
//
//  Classifies an HRESULT returned by IDXGISwapChain::Present (and related
//  D3D11/DXGI APIs) as "the device is gone and must be recreated", or not.
//  Used by the render-thread loop to trigger the application-level rebuild
//  path when the GPU goes away (driver reset, sleep/resume, eGPU unplugged,
//  user disabled adapter in Device Manager, etc.).
//
//  This helper is intentionally narrow: it covers the HRESULTs that
//  IDXGISwapChain::Present is documented to return for a lost device on
//  D3D11.  D3DDDIERR_* codes that can surface from GetDeviceRemovedReason
//  are deliberately NOT included here; that helper deals with Present's
//  return surface only.
//
////////////////////////////////////////////////////////////////////////////////

bool IsDeviceLost (HRESULT hr);
