# MatrixRain Development Guidelines

Auto-generated from all feature plans. Last updated: 2025-11-30

## Active Technologies
- C++26 (MSVC preview `/std:c++latest`) + Win32 API, DirectX 11 rendering stack, Project EHM macros, Microsoft C++ Native Unit Test Framework (002-add-screensaver-support)
- Windows Registry (`HKCU\Software\relmer\MatrixRain`) for per-user settings (002-add-screensaver-support)
- C++23 (`/std:c++latest`), Visual Studio 2026 (v18.x) + Direct3D 11.1, Direct2D 1.1, DirectWrite, Win32 API, ANSI VT escape sequences (003-runtime-help)
- N/A (no persistence; registry settings handled by existing `RegistrySettingsProvider`) (003-runtime-help)
- C++23 (`/std:c++latest`), Visual Studio 2026 (v18.x) + Win32 API (CopyFileW, DeleteFileW, ShellExecuteExW for UAC, rundll32 for desk.cpl), Windows Registry API (RegOpenKeyExW, RegQueryValueExW, RegSetValueExW, RegDeleteValueW) (005-screensaver-install)
- Windows Registry (`HKCU\Control Panel\Desktop`) — read/write for uninstall, read-only check for install (005-screensaver-install)

## Project Structure

```text
MatrixRain.sln

MatrixRain/
MatrixRainCore/
MatrixRainTests/
scripts/
assets/
```

## Commands

& "scripts/Invoke-MatrixRainBuild.ps1" -Target Build -Configuration Debug -Platform x64
& "scripts/Invoke-MatrixRainTests.ps1" -Configuration Debug -Platform x64 -Parallel

## Code Style

Follow MatrixRain formatting constitution: 5 blank lines between top-level constructs, column-aligned declarations, EHM macros for error handling, tests-first development in `MatrixRainCore`.

## Recent Changes
- 005-screensaver-install: Added C++23 (`/std:c++latest`), Visual Studio 2026 (v18.x) + Win32 API (CopyFileW, DeleteFileW, ShellExecuteExW for UAC, rundll32 for desk.cpl), Windows Registry API (RegOpenKeyExW, RegQueryValueExW, RegSetValueExW, RegDeleteValueW)
- 003-runtime-help: Added C++23 (`/std:c++latest`), Visual Studio 2026 (v18.x) + Direct3D 11.1, Direct2D 1.1, DirectWrite, Win32 API, ANSI VT escape sequences
- 002-add-screensaver-support: Added C++26 (MSVC preview `/std:c++latest`) + Win32 API, DirectX 11 rendering stack, Project EHM macros, Microsoft C++ Native Unit Test Framework

<!-- MANUAL ADDITIONS START -->
<!-- MANUAL ADDITIONS END -->
