# Quickstart: Screensaver Install/Uninstall

## Build

```powershell
# From repo root
& "C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe" MatrixRain.sln /p:Configuration=Debug /p:Platform=x64
```

## Test

```powershell
# Run all tests
pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\Invoke-MatrixRainTests.ps1 -Configuration Debug -Platform x64 -RunSettings MatrixRainTests.runsettings -Parallel
```

## Usage

### Install as screensaver
```powershell
# Copies MatrixRain.scr to System32, sets as active screensaver, opens Settings CPL
# UAC prompt will appear if not running as admin
.\x64\Debug\MatrixRain.exe /install
```

### Uninstall screensaver
```powershell
# Removes MatrixRain.scr from System32, clears registry if still active
.\x64\Debug\MatrixRain.exe /uninstall
```

### Show help (verify new switches appear)
```powershell
.\x64\Debug\MatrixRain.exe /?
```

## Manual Verification

1. **Install**: Run `/install` → UAC prompt → Screen Saver Settings opens with "MatrixRain" selected
2. **Verify file**: `Test-Path "$env:SystemRoot\System32\MatrixRain.scr"` → True
3. **Verify registry**: `Get-ItemProperty "HKCU:\Control Panel\Desktop" -Name "SCRNSAVE.EXE"` → contains `MatrixRain.scr`
4. **Uninstall**: Run `/uninstall` → success message
5. **Verify removed**: `Test-Path "$env:SystemRoot\System32\MatrixRain.scr"` → False
6. **Verify registry cleaned**: `SCRNSAVE.EXE` value is empty or missing

## Key Files

| File | Purpose |
|------|---------|
| `MatrixRainCore/ScreenSaverInstaller.h` | New: Install/uninstall class declaration |
| `MatrixRainCore/ScreenSaverInstaller.cpp` | New: Install/uninstall implementation |
| `MatrixRainCore/ScreenSaverMode.h` | Modified: Add Install, Uninstall enum values |
| `MatrixRainCore/ScreenSaverModeParser.cpp` | Modified: Parse multi-char switches |
| `MatrixRainCore/UsageText.cpp` | Modified: Add new switches to help text |
| `MatrixRain/main.cpp` | Modified: Dispatch Install/Uninstall modes |
| `MatrixRainTests/unit/ScreenSaverInstallerTests.cpp` | New: Unit tests |
| `MatrixRainTests/unit/ScreenSaverModeParserTests.cpp` | Modified: Add parsing tests |
