# Developer Quickstart: Matrix Rain

## Prerequisites

**Required Software**:
- **Visual Studio 2026** (version 18.x or later) with C++ desktop development workload
- **Windows 11** (latest updates)
- **Windows SDK** (latest version, installed with VS 2026)
- **Git** (for cloning repository)

**Hardware Requirements**:
- **GPU**: DirectX 11-capable (any GPU from ~2010 onwards)
- **RAM**: 4GB minimum (8GB recommended for development)
- **CPU**: x86-64 architecture (Intel/AMD 64-bit)

---

## Setup Steps

### 1. Clone Repository

```powershell
cd C:\repos
git clone <repository-url> MatrixRain
cd MatrixRain
```

### 2. Open Solution

- Launch **Visual Studio 2026**
- Open `MatrixRain.sln` from the repository root
- Wait for IntelliSense to load (status bar: "Ready")

**Solution Structure**:
```
MatrixRain.sln
├── MatrixRainCore (Static Library)  - Core engine library
├── MatrixRain (Windows Application) - Executable entry point
└── MatrixRainTests (Native Unit Tests) - Test project
```

### 3. Build Configuration

**Active Configuration**: Debug | x64

- Menu: **Build** → **Configuration Manager**
- Verify all projects set to **x64** platform
- Verify **Debug** configuration selected

**Build Order** (automatic via project dependencies):
1. MatrixRainCore.lib
2. MatrixRainTests.exe (references Core)
3. MatrixRain.exe (references Core)

### 4. Restore NuGet Packages (if applicable)

Visual Studio 2026 auto-restores packages on first build. Manual restore:

- Menu: **Tools** → **NuGet Package Manager** → **Restore NuGet Packages**

---

## Running Tests

### Command Line (Recommended)

```powershell
# Build tests
msbuild MatrixRain.sln /t:MatrixRainTests /p:Configuration=Debug /p:Platform=x64

# Run all tests
vstest.console.exe .\x64\Debug\MatrixRainTests.exe

# Run specific test class
vstest.console.exe .\x64\Debug\MatrixRainTests.exe /Tests:CharacterSetTests
```

### Visual Studio Test Explorer

1. Menu: **Test** → **Test Explorer** (or Ctrl+E, T)
2. Click **Run All** (green play button)
3. View results in Test Explorer window

**Expected Output**:
```
Total tests: ~50
Passed: ~50
Failed: 0
Skipped: 0
Duration: <5 seconds
```

### TDD Workflow

Per constitution, **all core library code requires tests first**:

1. Write test in `MatrixRainTests/unit/<module>/` (e.g., `AnimationTests.cpp`)
2. Run test → Verify it fails (red)
3. Implement minimal code in `MatrixRainCore/src/<module>/`
4. Run test → Verify it passes (green)
5. Refactor if needed, keeping tests green
6. Commit with message: `test: add <feature> tests` then `feat: implement <feature>`

---

## Running the Application

### From Visual Studio

1. Set **MatrixRain** as startup project (right-click → **Set as Startup Project**)
2. Press **F5** (Start Debugging) or **Ctrl+F5** (Start Without Debugging)
3. Application window opens (800×600 default)

**Expected Behavior**:
- Green matrix rain characters falling continuously
- 60fps animation (check via external FPS counter or frame timings)
- Responsive to keyboard input (see Controls below)

### From Command Line

```powershell
cd x64\Debug
.\MatrixRain.exe
```

### Performance Validation

**Frame Time Verification**:
- Add instrumentation: `OutputDebugString` in main loop with frame delta time
- Use **Visual Studio Performance Profiler**: Menu → **Debug** → **Performance Profiler** → **CPU Usage**
- Target: <16.67ms per frame (60fps), ideally <10ms

**Expected Metrics** (Debug build):
- Startup time: <1 second (from exe launch to first frame)
- Frame time: 5-10ms (60fps maintained)
- Memory usage: ~50MB (baseline + DirectX resources)

---

## Controls

### Keyboard Input

| Key | Action | Expected Result |
|-----|--------|-----------------|
| `+` | Increase density | More character streaks spawn (level 1→10) |
| `-` | Decrease density | Fewer streaks active (level 10→1) |
| `Alt+Enter` | Toggle fullscreen | Switch windowed ↔ fullscreen (<200ms transition) |
| `Escape` | Exit application | Immediate shutdown (Debug mode) |

### Mouse Input

Not implemented in MVP (reserved for future camera controls).

---

## Debugging

### Common Issues

**Issue**: Application crashes on startup with "D3D11CreateDevice failed"

**Solution**:
- Verify GPU drivers updated (Windows Update → Optional Updates)
- Check DirectX 11 support: Run `dxdiag.exe` → Display tab → Feature Levels (must include 11.0+)

**Issue**: Tests fail with "Unable to load MatrixRainCore.dll"

**Solution**:
- Ensure MatrixRainCore project built successfully
- Check output directory: `x64\Debug\MatrixRainCore.lib` exists
- Rebuild solution: Menu → **Build** → **Rebuild Solution**

**Issue**: Character rendering shows garbage/missing glyphs

**Solution**:
- Verify `CharacterSet` initialization succeeded (check debug output)
- Ensure DirectWrite fonts available: Install missing fonts via Windows Settings
- Re-run texture atlas generation (see `CharacterSet::InitializeAtlas()`)

### Breakpoint Locations

**Startup Flow**:
- `main.cpp:WinMain()` - Entry point
- `MatrixRainCore/Application::Initialize()` - D3D11 device creation
- `CharacterSet::InitializeAtlas()` - Texture atlas generation

**Animation Loop**:
- `AnimationSystem::Update(float deltaTime)` - Streak spawning/updating
- `RenderSystem::Render()` - DirectX draw calls

**Input Handling**:
- `InputSystem::ProcessKeyDown(VK_ADD)` - Density increase
- `InputSystem::ProcessKeyDown(VK_RETURN + Alt)` - Fullscreen toggle

---

## IntelliSense Configuration

Each project has a `.vscode/c_cpp_properties.json` (or VS equivalent) for per-project settings:

**MatrixRainCore**:
- Include paths: `include/`, Windows SDK, DirectX SDK
- C++ standard: `c++latest` (C++23)
- Defines: `UNICODE`, `_UNICODE`, `WIN32_LEAN_AND_MEAN`

**MatrixRainTests**:
- Additional includes: `MatrixRainCore/include`, Microsoft Unit Test Framework headers
- Defines: Same as Core + `_DEBUG`

**Verify IntelliSense**:
- Open any `.cpp` file
- Status bar (bottom right): Should show "C++23" (not "C++17")
- Hover over DirectX types (e.g., `ID3D11Device*`) → Tooltip shows definition

---

## Project Structure Overview

```
MatrixRain/
├── .specify/               # Speckit documentation
│   └── memory/constitution.md
├── specs/
│   └── 001-matrix-rain/
│       ├── spec.md         # Feature specification
│       ├── plan.md         # Implementation plan
│       ├── research.md     # Technical decisions
│       └── data-model.md   # Entity design
├── src/
│   ├── MatrixRainCore/     # Core library
│   │   ├── include/matrixrain/  # Public API headers
│   │   └── src/
│   │       ├── animation/
│   │       ├── rendering/
│   │       ├── input/
│   │       ├── state/
│   │       └── characters/
│   ├── MatrixRain/         # Executable
│   │   └── main.cpp        # WinMain entry point (<10 lines)
│   └── MatrixRainTests/    # Tests
│       └── unit/
│           ├── animation/
│           ├── rendering/
│           ├── input/
│           ├── state/
│           └── characters/
└── MatrixRain.sln          # VS 2026 solution
```

---

## Next Steps

1. **Explore Codebase**:
   - Read `specs/001-matrix-rain/spec.md` for feature requirements
   - Review `data-model.md` for entity relationships
   - Check `research.md` for technical decisions (DirectX 11, texture atlas strategy)

2. **Run Example Tests**:
   - Open `MatrixRainTests/unit/characters/CharacterSetTests.cpp`
   - Run tests in Test Explorer
   - Observe assertion patterns (`Assert::AreEqual`, `Assert::IsTrue`)

3. **Make First Change**:
   - Follow TDD workflow above
   - Example: Add test for `CharacterStreak::Update(deltaTime)` fade timing
   - Implement in `MatrixRainCore/src/animation/CharacterStreak.cpp`
   - Commit per constitution commit discipline

4. **Verify Performance**:
   - Use **Visual Studio Performance Profiler** on Release build
   - Target: 60fps maintained with 500 active streaks (max density)
   - Investigate hotspots if frame time >16ms

---

## Additional Resources

**Documentation**:
- Constitution: `.specify/memory/constitution.md` (project principles)
- DirectX 11 Guide: [Microsoft Learn](https://learn.microsoft.com/en-us/windows/win32/direct3d11/)
- DirectWrite: [Microsoft Learn](https://learn.microsoft.com/en-us/windows/win32/directwrite/)

**Tools**:
- **RenderDoc**: Free DirectX debugging tool (capture frame, inspect draw calls)
- **PIX on Windows**: Microsoft GPU profiler for DirectX applications
- **Visual Studio Graphics Debugger**: Built-in DirectX debugging (F5 → Graphics → Start Diagnostics)

**Getting Help**:
- Check constitution for decision guidelines
- Review spec.md for requirements clarification
- Search research.md for resolved technical questions
