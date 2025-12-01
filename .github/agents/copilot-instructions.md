# MatrixRain Development Guidelines

Auto-generated from all feature plans. Last updated: 2025-11-30

## Active Technologies
- C++26 (MSVC preview `/std:c++latest`) + Win32 API, DirectX 11 rendering stack, Project EHM macros, Microsoft C++ Native Unit Test Framework (002-add-screensaver-support)
- Windows Registry (`HKCU\Software\relmer\MatrixRain`) for per-user settings (002-add-screensaver-support)

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
- 002-add-screensaver-support: Added C++26 (MSVC preview `/std:c++latest`) + Win32 API, DirectX 11 rendering stack, Project EHM macros, Microsoft C++ Native Unit Test Framework

<!-- MANUAL ADDITIONS START -->
<!-- MANUAL ADDITIONS END -->
