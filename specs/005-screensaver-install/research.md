# Research: Screensaver Install/Uninstall

## R1: Multi-Character Switch Parsing

**Decision**: Extend `ParseCommandLine` to collect all alphabetic characters after the prefix, then match against known multi-character commands ("install", "uninstall") before falling through to single-character matching.

**Rationale**: The current parser extracts a single character (`cmd = towlower(*pszCommandLine)`) and switches on it. For "install"/"uninstall", we need to read the full word first. Since single-char switches (`s`, `c`, `p`, `a`, `?`) are a subset of this approach (1-character words), the parser can handle both uniformly.

**Approach**:
1. After skipping the prefix (`/` or `-`), collect all consecutive alphabetic characters into a `wstring_view`
2. Check if the collected string matches "install" or "uninstall" (case-insensitive)
3. If no multi-char match, fall through to the existing single-character switch statement
4. Non-alpha characters like `?` are not alphabetic, so they skip the multi-char path naturally

**Alternatives considered**:
- Checking only the first character (`i` for install, `u` for uninstall) — rejected because `i` could conflict with future switches, and "uninstall" starts with the same char as potential future commands.

## R2: desk.cpl InstallScreenSaver Invocation

**Decision**: Use `rundll32.exe desk.cpl,InstallScreenSaver <path>` to invoke the standard Windows screensaver installation.

**Rationale**: This is the exact mechanism Windows Explorer uses when you right-click an `.scr` file and choose "Install". The registry handler at `HKCR\scrfile\shell\install\command` confirms: `rundll32.exe desk.cpl,InstallScreenSaver %l`. This function:
1. Sets `SCRNSAVE.EXE` registry value to the provided `.scr` path
2. Sets `ScreenSaveActive` to "1"
3. Opens the Screen Saver Settings dialog with the specified screensaver selected

**Implementation**: Use `ShellExecuteW` or `CreateProcessW` to invoke `rundll32.exe desk.cpl,InstallScreenSaver <System32\MatrixRain.scr>`.

**Alternatives considered**:
- Calling `desk.cpl` via `rundll32` with LoadLibrary/GetProcAddress — rejected, unnecessary complexity when ShellExecute is simpler and matches the Explorer behavior exactly.

## R3: UAC Self-Elevation Pattern

**Decision**: Use `ShellExecuteExW` with `lpVerb = L"runas"` to re-launch the process with elevation, passing through the original command-line switch.

**Rationale**: This is the standard Windows pattern for requesting elevation. When the process detects it's not elevated, it re-launches itself with "runas" which triggers the UAC dialog. The elevated process then performs the privileged operation.

**Approach**:
1. Check elevation status via `OpenProcessToken` + `GetTokenInformation(TokenElevation)`
2. If not elevated, call `ShellExecuteExW` with `lpVerb = L"runas"`, `lpFile = <current exe path>`, `lpParameters = <original switch>`
3. Return immediately (the elevated instance handles the work)
4. If already elevated, proceed with the operation

**Alternatives considered**:
- Embedding a manifest requiring elevation — rejected because it would force UAC on every launch, including normal screensaver operation.
- Using `ICMLuaUtil` COM interface — rejected, undocumented and unreliable.

## R4: Uninstall Registry Cleanup

**Decision**: Use `RegOpenKeyExW` / `RegQueryValueExW` / `RegSetValueExW` / `RegDeleteValueW` on `HKCU\Control Panel\Desktop` to clean up screensaver registry entries during uninstall.

**Rationale**: Unlike install (where `desk.cpl` handles registry), uninstall has no Windows helper. We must:
1. Read `SCRNSAVE.EXE` to check if it points to our `.scr` file
2. If it does: delete `SCRNSAVE.EXE` and set `ScreenSaveActive` to `"0"`
3. If it points to a different screensaver: leave registry untouched, only delete the `.scr` file

**Implementation**: Compare the registry value (case-insensitive path comparison) against the expected `%SystemRoot%\System32\MatrixRain.scr` path. Use `GetSystemDirectoryW` to get the actual System32 path for reliable comparison.

**Alternatives considered**:
- Always clearing the registry regardless of current screensaver — rejected, would break the user's configuration if they switched to a different screensaver after installing MatrixRain.

## R5: Self-Copy Mechanism

**Decision**: Use `GetModuleFileNameW(NULL, ...)` to get the running executable's path, then `CopyFileW` to copy it to `%SystemRoot%\System32\MatrixRain.scr`.

**Rationale**: The running `.exe` copies itself. An `.scr` is byte-identical to an `.exe` — Windows only cares about the file extension for screensaver discovery. `CopyFileW` with `bFailIfExists = FALSE` handles both fresh install and upgrade (overwrite) cases.

**Alternatives considered**:
- Using `MoveFileExW` — rejected, we want to keep the original executable in place.
- Reading from an adjacent `.scr` build artifact — rejected per clarification: self-contained copy is simpler and works regardless of distribution method.
