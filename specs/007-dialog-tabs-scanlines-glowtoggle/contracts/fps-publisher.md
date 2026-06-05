# Contract: Live FPS Publisher (MonitorRenderContext → Dialog)

## Producer (render thread, `MonitorRenderContext`)

```cpp
class MonitorRenderContext
{
public:
    // Reader access (called by dialog thread):
    float  GetPublishedFps  () const noexcept { return m_publishedFps   .load (std::memory_order_relaxed); }
    bool   HasPublishedFps  () const noexcept { return m_hasPublishedFps.load (std::memory_order_relaxed); }

private:
    std::atomic<float>  m_publishedFps     {0.0f};
    std::atomic<bool>   m_hasPublishedFps  {false};

    // ... existing v1.4 members ...
};
```

Render-thread write (added once, in the per-frame tick just after the
existing `m_fpsCounter.Tick()`):

```cpp
m_fpsCounter.Tick();

float currentFps = m_fpsCounter.GetFps();
m_publishedFps   .store (currentFps, std::memory_order_relaxed);
m_hasPublishedFps.store (true,       std::memory_order_relaxed);
```

## Consumer (dialog thread, property-sheet 1 Hz `WM_TIMER`)

The consumer needs *the primary monitor's* FPS (FR-010). The application
already maintains an ordered list of `MonitorRenderContext` per the v1.4
multimon work; the primary is identified by
`MonitorInfo::IsPrimary == true`. The dialog timer handler walks the
list, picks the primary, and reads via the lock-free accessors:

```cpp
const MonitorRenderContext * pPrimary = m_pApplication->GetPrimaryRenderContext();
bool                         hasFps   = false;
float                        fpsValue = 0.0f;


if (pPrimary != nullptr)
{
    hasFps   = pPrimary->HasPublishedFps();
    fpsValue = pPrimary->GetPublishedFps();
}
```

Hot-removal of the primary monitor between ticks (Edge Case "GPU adapter
switch while dialog is open"): `GetPrimaryRenderContext` may return
`nullptr` for one or two ticks while the multimon gate rebuilds; the
handler falls back to displaying `--` for that tick. Within ~1 second
the rebuilt primary publishes its first frame and the display resumes
(satisfies the "within ~1 second" Edge Case requirement).

## Memory ordering rationale

The producer and consumer are not synchronising any data outside the
single float value; `std::memory_order_relaxed` on both sides is
correct. There is no acquire/release pairing because:
- A torn read is impossible on x64 and ARM64 (4-byte aligned float
  load/store is atomic at the hardware level; `std::atomic<float>` is
  `is_always_lock_free` on these platforms).
- A stale read is acceptable — the consumer reads at 1 Hz, the producer
  writes at ~60 Hz; "stale" means "from up to 16 ms ago", which is
  invisible to a human watching a 1 Hz display.

## Sentinel behaviour

`m_hasPublishedFps` starts `false`. The first frame's `Tick()` writes
both fields. Once `true`, it stays `true` for the lifetime of the
context (it is never reset). The sentinel is used internally by the
dialog timer to distinguish "no reading yet" from "real 0 fps", but
the rendered title shows `0 fps` in both cases per FR-012 (the format
string is uniformly `%u`/`%u`, with no placeholder token in the
output). The sentinel matters for downstream logic that wants to
suppress action until a real reading arrives.
