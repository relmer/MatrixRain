# Contract: `IDisplayLuminanceProvider`

**Module**: `MatrixRainCore/IDisplayLuminanceProvider.h` (interface),
`WindowsDisplayLuminanceProvider.{h,cpp}` (production),
`InMemoryDisplayLuminanceProvider.h` (test fake).

**Purpose**: Isolate the OS queries (DXGI output description, DisplayConfig
SDR white level, scRGB present support) behind a seam, the same way
`IAdapterProvider` / `WindowsAdapterProvider` isolate adapter enumeration, so
mode selection and luminance math are testable with a fake.

## Interface

```text
class IDisplayLuminanceProvider
{
public:
    virtual ~IDisplayLuminanceProvider() = default;

    // Snapshot for the output that currently contains the swap chain's window.
    // Never throws; on any failure returns hdrEnabled = false,
    // scRgbSupported = false and sdrWhiteNits = 80 so the caller falls back
    // to SDR (FR-015).
    virtual DisplayLuminance Query (IDXGISwapChain1 * pSwapChain) noexcept = 0;

    // True when the cached DXGI factory no longer reflects the display
    // configuration; the next Query refreshes it.
    virtual bool IsStale() noexcept = 0;
};
```

## Behavior

- `Query` is cheap enough to call at 1 Hz per monitor from the render thread
  (target < 1 ms). It must not block on the UI thread.
- DXGI↔DisplayConfig pairing uses `DXGI_OUTPUT_DESC1::DeviceName` against the
  DisplayConfig source GDI device name. If no match is found, `sdrWhiteNits`
  is 80.
- `hdrEnabled` is true when the output color space is
  `RGB_FULL_G2084_NONE_P2020` (Windows HDR is on for that monitor).
- `scRgbSupported` is true when the swap chain reports present support for
  `RGB_FULL_G10_NONE_P709`. The two are reported separately, and
  `SelectOutputMode` combines them (see [color-math.md](color-math.md)).

## Fake for tests

`InMemoryDisplayLuminanceProvider`, following the `InMemoryAdapterProvider` /
`InMemoryMonitorProvider` pattern, returns scripted `DisplayLuminance` values and
a scripted stale flag, so tests can drive SDR→HDR→SDR transitions and white
level changes.
