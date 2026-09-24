<#
.SYNOPSIS
    Walks through the spec 008 Phase 2 hardware checks that need a person
    watching: mixed HDR and SDR monitors, the screen saver preview, recovery
    from a GPU restart, and Auto HDR. Records each result to a Markdown file.

.DESCRIPTION
    Each step says what to set up, launches what is needed, and asks for
    Pass, Fail or Skip and an optional note. Nothing that changes the system
    runs without asking first: installing the build as the screen saver and
    restarting the display adapter each prompt, and each is elevated by
    Windows' own UAC prompt.

    Remote Desktop (quickstart check 8) is not covered: MatrixRain is not
    meant to run in a Remote Desktop session.

.PARAMETER Exe
    The MatrixRain.exe to test. Defaults to the Release x64 build.

.PARAMETER ResultsPath
    Where to write the results. Defaults to
    specs/008-hdr-linear-rendering/phase2-checks.md.

.EXAMPLE
    .\scripts\Test-HdrPhase2.ps1
#>
param(
    [string] $Exe         = (Join-Path $PSScriptRoot '..\x64\Release\MatrixRain.exe'),
    [string] $ResultsPath = (Join-Path $PSScriptRoot '..\specs\008-hdr-linear-rendering\phase2-checks.md')
)

$ErrorActionPreference = 'Stop'

$Exe     = (Resolve-Path $Exe).Path
$harness = Join-Path $PSScriptRoot '..\x64\Calibration\HdrCalibration.exe'
$results = [System.Collections.Generic.List[object]]::new()





function Write-Step ([string] $title)
{
    Write-Host ''
    Write-Host ('=' * 72) -ForegroundColor DarkGray
    Write-Host $title -ForegroundColor Cyan
    Write-Host ('=' * 72) -ForegroundColor DarkGray
}





function Confirm-Action ([string] $question)
{
    $answer = Read-Host "$question [y/N]"

    return $answer -match '^(y|yes)$'
}





function Read-Result ([string] $check, [string] $expected)
{
    Write-Host ''
    Write-Host "Expected: $expected" -ForegroundColor Yellow

    do
    {
        $answer = (Read-Host 'Result: (p)ass, (f)ail, (s)kip').Trim().ToLowerInvariant()
    }
    while ($answer -notin @('p', 'f', 's', 'pass', 'fail', 'skip'))

    $outcome = switch ($answer.Substring(0, 1)) { 'p' { 'Pass' } 'f' { 'Fail' } 's' { 'Skip' } }
    $note    = Read-Host 'Note (optional)'

    $results.Add([pscustomobject] @{ Check = $check; Result = $outcome; Note = $note })
}





function Show-Monitors
{
    if (Test-Path $harness)
    {
        Write-Host 'Monitors as DXGI reports them:'
        & $harness --adapter hardware --mode luminance 2>&1 |
            Select-String '^\s+\\\\' |
            ForEach-Object { Write-Host "  $($_.Line.Trim())" }
    }
    else
    {
        Write-Host '(Build the HdrCalibration harness to list each monitor''s HDR state here.)'
    }
}





function Start-Rain ([string] $arguments)
{
    Write-Host "Starting: MatrixRain.exe $arguments"
    Write-Host 'Move the mouse or press a key over the rain to exit it when done.'

    Start-Process -FilePath $Exe -ArgumentList $arguments -Wait
}





################################################################################
#
#  Check 5: HDR and SDR monitors together
#
################################################################################

Write-Step 'Check 5: one monitor in HDR, one in SDR'

Write-Host 'Turn HDR off on ONE of your monitors (the Settings page opens now),'
Write-Host 'leave it on for the other, then come back here.'
Start-Process 'ms-settings:display-hdr'
Read-Host 'Press Enter when one monitor has HDR on and the other off'

Show-Monitors
Start-Rain '/s'

Read-Result 'Check 5: HDR and SDR monitors together' `
            'Rain on both monitors; trail brightness, glyph shape, glow and scanlines look the same on both'

Write-Host 'Turn HDR back on for that monitor before the next check.'
Start-Process 'ms-settings:display-hdr'
Read-Host 'Press Enter when both monitors have HDR on again'





################################################################################
#
#  Check 7: the screen saver preview
#
################################################################################

Write-Step 'Check 7: the preview in the Windows screen saver dialog'

$installed = (Get-ItemProperty 'HKCU:\Control Panel\Desktop' -Name 'SCRNSAVE.EXE' -ErrorAction SilentlyContinue).'SCRNSAVE.EXE'

Write-Host "Installed screen saver: $(if ($installed) { $installed } else { '(none)' })"
Write-Host 'The preview runs the INSTALLED copy, so it has to be this build.'

if (Confirm-Action 'Install this build as the screen saver now? (Windows asks for elevation)')
{
    Start-Process -FilePath $Exe -ArgumentList '/install' -Wait
}

Start-Process 'control.exe' -ArgumentList 'desk.cpl,,@screensaver'

Read-Result 'Check 7: screen saver preview' `
            'The small preview shows the rain in SDR, as v1.6 did, with no error'





################################################################################
#
#  Check 9: recovery from a GPU restart
#
################################################################################

Write-Step 'Check 9: recover in HDR after the display adapter restarts'

$adapters = @(Get-PnpDevice -Class Display -Status OK -ErrorAction SilentlyContinue)

if ($adapters.Count -eq 0)
{
    Write-Host 'No display adapters found; skipping.'
    $results.Add([pscustomobject] @{ Check = 'Check 9: GPU restart'; Result = 'Skip'; Note = 'No adapter found' })
}
else
{
    for ($i = 0; $i -lt $adapters.Count; ++$i)
    {
        Write-Host "  [$i] $($adapters[$i].FriendlyName)"
    }

    $choice = Read-Host 'Which adapter renders the rain? (the one your HDR monitors are connected to)'

    if ($choice -match '^\d+$' -and [int] $choice -lt $adapters.Count)
    {
        $adapter = $adapters[[int] $choice]

        Write-Host 'The rain starts windowed with the settings dialog. When it is running,'
        Write-Host "come back here to restart '$($adapter.FriendlyName)'. The screens go"
        Write-Host 'black for a few seconds while the driver restarts.'

        $rain = Start-Process -FilePath $Exe -ArgumentList '/c' -PassThru

        Start-Sleep -Seconds 3

        if (Confirm-Action "Restart '$($adapter.FriendlyName)' now? (Windows asks for elevation)")
        {
            Start-Process -FilePath 'pnputil.exe' `
                          -ArgumentList '/restart-device', "`"$($adapter.InstanceId)`"" `
                          -Verb RunAs -Wait
        }

        Read-Result 'Check 9: GPU restart' `
                    'The rain comes back within a few seconds, still in HDR (heads above SDR white), with no crash'

        if (-not $rain.HasExited)
        {
            Stop-Process -Id $rain.Id -ErrorAction SilentlyContinue
        }
    }
    else
    {
        $results.Add([pscustomobject] @{ Check = 'Check 9: GPU restart'; Result = 'Skip'; Note = 'No adapter chosen' })
    }
}





################################################################################
#
#  Check 10: Auto HDR
#
################################################################################

Write-Step 'Check 10: Auto HDR turned on in Windows'

Write-Host 'Turn Auto HDR ON on the HDR settings page (it opens now), then come back.'
Start-Process 'ms-settings:display-hdr'
Read-Host 'Press Enter when Auto HDR is on'

Start-Rain '/s'

Read-Result 'Check 10: Auto HDR' `
            'Looks the same as with Auto HDR off: no extra brightening or color change on top of ours'

Write-Host 'Set Auto HDR back the way you like it.'





################################################################################
#
#  Results
#
################################################################################

$commit = (git -C $PSScriptRoot rev-parse --short HEAD 2>$null)
$lines  = [System.Collections.Generic.List[string]]::new()

$lines.Add('# Phase 2 hardware checks')
$lines.Add('')
$lines.Add("Run $(Get-Date -Format 'yyyy-MM-dd HH:mm') on build $commit with ``scripts/Test-HdrPhase2.ps1``.")
$lines.Add('Remote Desktop (quickstart check 8) is out of scope: MatrixRain is not meant to run over RDP.')
$lines.Add('')
$lines.Add('| Check | Result | Note |')
$lines.Add('|---|---|---|')

foreach ($r in $results)
{
    $lines.Add("| $($r.Check) | $($r.Result) | $($r.Note -replace '\|', '/') |")
}

Set-Content -Path $ResultsPath -Value $lines -Encoding utf8

Write-Host ''
Write-Host "Results written to $ResultsPath" -ForegroundColor Green
$results | Format-Table -AutoSize
