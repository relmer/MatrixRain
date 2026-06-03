# Contract — `IAdapterProvider` and `AdapterInfo`

**Feature**: `006-multimon-gpu-efficiency`
**Header**: `MatrixRainCore\IAdapterProvider.h` (new)

This contract defines the new interface and value type used to enumerate GPU adapters for the GPU-selection feature. It mirrors the existing `IMonitorProvider`/`WindowsMonitorProvider`/`InMemoryMonitorProvider` pattern.

---

## `struct AdapterInfo`

```cpp
struct AdapterInfo
{
    std::wstring  m_description;     // Human-readable adapter name (DXGI_ADAPTER_DESC1::Description)
    LUID          m_luid;            // Stable-within-session id (DXGI_ADAPTER_DESC1::AdapterLuid)
    unsigned int  m_dedicatedVramMb; // DedicatedVideoMemory in MB
    bool          m_isSoftware;      // True for Microsoft Basic Render Driver / WARP
    bool          m_isDefault;       // True iff this is the system default rendering adapter
};
```

**Invariants**:
- `m_description.empty()` iff DXGI returned an empty description; consumers MUST treat this as "unknown" and exclude such an adapter from any user-visible list.
- At most one `AdapterInfo` in any returned list has `m_isDefault == true`. Exactly one is required when at least one non-software adapter exists.

---

## `class IAdapterProvider` *(abstract)*

```cpp
class IAdapterProvider
{
public:
    virtual ~IAdapterProvider() = default;

    // Enumerate all rendering-capable GPU adapters currently present on the
    // system. Software adapters (DXGI_ADAPTER_FLAG_SOFTWARE) MUST be excluded.
    // Returns an empty vector if no non-software adapters exist (the caller
    // is then expected to use the system default-adapter path).
    virtual std::vector<AdapterInfo> EnumerateAdapters() const = 0;
};
```

**Contract**:
- `EnumerateAdapters()` is pure and side-effect-free; it MAY be called multiple times during one process lifetime and MUST return current state each call.
- The order of the returned vector is implementation-defined; consumers MUST find the default entry via `m_isDefault`, not by index.

---

## Implementations

### `WindowsAdapterProvider`

- Uses `CreateDXGIFactory1` to obtain `IDXGIFactory1` for the basic enumeration.
- Uses `CreateDXGIFactory2` (with `DXGI_CREATE_FACTORY_DEBUG` in `_DEBUG` builds) and `QueryInterface` for `IDXGIFactory6` to identify the system default adapter via `EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&defaultAdapter))`.
- For each `EnumAdapters1` result: call `GetDesc1`, skip if `(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0`, populate one `AdapterInfo`, mark `m_isDefault = (desc.AdapterLuid == defaultAdapter.GetDesc1().AdapterLuid)`.
- On any DXGI failure HRESULT, log via existing `Console`/EHM macros and return an empty vector (callers then use the default-adapter path).

### `InMemoryAdapterProvider` *(test seam)*

- Constructor takes a `std::vector<AdapterInfo>` and stores it.
- `EnumerateAdapters()` returns a copy of the stored vector.
- No DXGI dependency; usable freely in unit tests.

---

## Pure helpers consuming `AdapterInfo`

### `std::optional<LUID> ResolveAdapter(const std::vector<AdapterInfo>& adapters, const std::wstring& savedDescription)`

- Returns `nullopt` iff `savedDescription` is empty, or no adapter in `adapters` has a matching `m_description`. (Caller then creates the device with `nullptr` + `D3D_DRIVER_TYPE_HARDWARE`.)
- Returns the matching adapter's `m_luid` otherwise. (Caller then looks up the adapter via `EnumAdapterByLuid` and creates the device with `D3D_DRIVER_TYPE_UNKNOWN`.)
- Software adapters MUST already be excluded by the provider; this helper does not re-filter them.

### `std::wstring FormatAdapterLabel(const AdapterInfo& adapter)`

- Returns `m_description` unchanged if `m_isDefault == false`.
- Returns `m_description + L" (default)"` if `m_isDefault == true`.

Both helpers are pure; unit-tested via `InMemoryAdapterProvider` seeds.

---

## Device-creation contract change

`RenderSystem::Initialize(HWND hwnd, int width, int height)` becomes `RenderSystem::Initialize(HWND hwnd, int width, int height, std::optional<LUID> adapterLuid)`.

- `adapterLuid == nullopt` → existing behavior: `D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, …)`.
- `adapterLuid` present → look up via `factory6->EnumAdapterByLuid(*adapterLuid, IID_PPV_ARGS(&adapter))`; on success `D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, …)`; on lookup failure (adapter disappeared between enumeration and creation), fall back to `nullptr` + `D3D_DRIVER_TYPE_HARDWARE` and log via EHM.

The default-adapter fallback path is exercised in production by FR-014 (saved adapter not present at startup).
