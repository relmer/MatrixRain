<#
    Manual test for the drag-across-monitors freeze (a UI-thread wait on the
    render lock while the render thread sits in a flip-model Present, which
    DWM only breaks after 5 s). Launches the exe windowed, moves its window
    between two monitors with SetWindowPos, which fires WM_DPICHANGED exactly
    as a drag does, and after each move times how long the UI thread takes to
    answer a WM_NULL. Healthy: every SetWindowPos returns in well under a
    second and every WM_NULL is answered in a few ms. If the UI thread stops
    answering, grabs every thread's stack with cdb (non-invasive) and saves it
    beside this script.

    Needs two monitors, ideally at different scale factors. The exe must be
    built with a PDB beside it for the stacks to have symbols.

    usage: Test-DpiDrag.ps1 -Exe <path to MatrixRain.exe> -Label <name> [-Rounds 6]
#>
param(
    [Parameter(Mandatory)] [string] $Exe,
    [Parameter(Mandatory)] [string] $Label,
    [int] $Rounds = 6
)

$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public static class Win
{
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct MONITORINFO { public int cbSize; public RECT rcMonitor; public RECT rcWork; public uint dwFlags; }

    delegate bool MonitorEnumProc(IntPtr hMonitor, IntPtr hdc, ref RECT rect, IntPtr data);

    [DllImport("user32.dll")] static extern bool EnumDisplayMonitors(IntPtr hdc, IntPtr clip, MonitorEnumProc proc, IntPtr data);
    [DllImport("user32.dll")] static extern bool GetMonitorInfo(IntPtr hMonitor, ref MONITORINFO info);
    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] public static extern IntPtr SendMessageTimeout(IntPtr hWnd, uint msg, IntPtr w, IntPtr l, uint flags, uint timeout, out IntPtr result);
    [DllImport("user32.dll")] public static extern bool IsHungAppWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);

    public static List<RECT> Monitors()
    {
        var list = new List<RECT>();
        EnumDisplayMonitors(IntPtr.Zero, IntPtr.Zero, (IntPtr h, IntPtr dc, ref RECT r, IntPtr d) =>
        {
            var info = new MONITORINFO(); info.cbSize = Marshal.SizeOf(typeof(MONITORINFO));
            GetMonitorInfo(h, ref info);
            list.Add(info.rcMonitor);
            return true;
        }, IntPtr.Zero);
        return list;
    }
}
'@

# Per-monitor-v2 awareness so every coordinate below is a physical pixel.
[void][Win]::SetProcessDpiAwarenessContext([IntPtr]::new(-4))

$monitors = [Win]::Monitors()
"monitors (physical):"
$monitors | ForEach-Object { "  L={0} T={1} R={2} B={3}" -f $_.Left, $_.Top, $_.Right, $_.Bottom }
if ($monitors.Count -lt 2) { throw 'need two monitors' }

$proc = Start-Process -FilePath $Exe -PassThru
Start-Sleep -Seconds 3
$proc.Refresh()
$hwnd = $proc.MainWindowHandle
if ($hwnd -eq [IntPtr]::Zero) { Stop-Process -Id $proc.Id -Force; throw 'no main window' }
"pid $($proc.Id) hwnd $hwnd"

$SWP_NOZORDER = 0x0004; $SWP_NOACTIVATE = 0x0010; $SMTO_ABORTIFHUNG = 0x0002
$size = 1200

function Ping([IntPtr] $h) {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $out = [IntPtr]::Zero
    $r = [Win]::SendMessageTimeout($h, 0, [IntPtr]::Zero, [IntPtr]::Zero, $SMTO_ABORTIFHUNG, 3000, [ref] $out)
    $sw.Stop()
    if ($r -eq [IntPtr]::Zero) { return -1 } else { return $sw.ElapsedMilliseconds }
}

$worst = 0; $hung = $false
$cdb = "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe"
$dump = Join-Path $PSScriptRoot "stacks-$Label.txt"

for ($round = 1; $round -le $Rounds; $round++) {
    foreach ($m in $monitors) {
        $x = $m.Left + [int](($m.Right - $m.Left - $size) / 2)
        $y = $m.Top  + [int](($m.Bottom - $m.Top - $size) / 2)
        $t0 = [System.Diagnostics.Stopwatch]::StartNew()
        [void][Win]::SetWindowPos($hwnd, [IntPtr]::Zero, $x, $y, $size, $size, $SWP_NOZORDER -bor $SWP_NOACTIVATE)
        $moveMs = $t0.ElapsedMilliseconds
        Start-Sleep -Milliseconds 700
        $ping = Ping $hwnd
        $rect = New-Object 'Win+RECT'; [void][Win]::GetWindowRect($hwnd, [ref] $rect)
        "round $round -> monitor L=$($m.Left): SetWindowPos took $moveMs ms, WM_NULL answered in $ping ms, hung=$([Win]::IsHungAppWindow($hwnd)), window now L=$($rect.Left) T=$($rect.Top) W=$($rect.Right - $rect.Left)"
        if ($ping -lt 0 -or $ping -gt 1000) {
            $worst = [Math]::Max($worst, [Math]::Abs($ping))
            if (-not $hung -and (Test-Path $cdb)) {
                $hung = $true
                "  UI thread not answering: capturing stacks to $dump"
                & $cdb -pv -p $proc.Id -y "srv*;$(Split-Path $Exe)" -c "~*k;q" 2>&1 | Out-File $dump -Encoding utf8
            }
        }
        else { $worst = [Math]::Max($worst, $ping) }
    }
}

"worst WM_NULL response: $worst ms  (-1 = timed out at 3 s)"
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
