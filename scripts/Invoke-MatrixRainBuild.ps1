[CmdletBinding()]
param(
    [ValidateSet('Build', 'Clean', 'Rebuild', 'BuildAllRelease', 'CleanAll', 'RebuildAllRelease')]
    [string]$Target = 'Build',

    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64', 'ARM64', 'Auto')]
    [string]$Platform = 'Auto'
)

# Resolve 'Auto' platform to actual architecture
if ($Platform -eq 'Auto') {
    if ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq [System.Runtime.InteropServices.Architecture]::Arm64) {
        $Platform = 'ARM64'
    } else {
        $Platform = 'x64'
    }
}

$ErrorActionPreference = 'Stop'





$script:BuildResults = @()





function Add-BuildResult {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Configuration,
        [Parameter(Mandatory = $true)]
        [string]$Platform,
        [Parameter(Mandatory = $true)]
        [string]$Target,
        [Parameter(Mandatory = $true)]
        [ValidateSet('Succeeded', 'Failed', 'Skipped', 'Warning')]
        [string]$Status,
        [int]$ExitCode = 0,
        [TimeSpan]$Duration,
        [string]$Message
    )

    $script:BuildResults += [PSCustomObject]@{
        Configuration = $Configuration
        Platform      = $Platform
        Target        = $Target
        Status        = $Status
        ExitCode      = $ExitCode
        Duration      = $Duration
        Message       = $Message
    }
}





function Write-BuildSummary {
    if (-not $script:BuildResults -or $script:BuildResults.Count -eq 0) {
        return
    }

    Write-Host ''
    Write-Host 'SUMMARY' -ForegroundColor White

    foreach ($r in $script:BuildResults) {
        $statusText = $r.Status.ToUpperInvariant()
        $label      = "{0}|{1} {2}" -f $r.Configuration, $r.Platform, $r.Target
        $timeText   = ''
        $details    = ''

        if ($r.Duration -and $r.Duration -gt [TimeSpan]::Zero -and $r.Status -ne 'Skipped') {
            $minutes  = [int][Math]::Floor($r.Duration.TotalMinutes)
            $timeText = " ({0:00}:{1:00}.{2:000})" -f $minutes, $r.Duration.Seconds, $r.Duration.Milliseconds
        }

        if ($r.Message) {
            $details = " - {0}" -f $r.Message
        }
        elseif ($r.ExitCode -ne 0) {
            $details = " - ExitCode {0}" -f $r.ExitCode
        }

        $line = "{0,-20} {1}{2}{3}" -f $label, $statusText, $timeText, $details

        switch ($r.Status) {
            'Succeeded' { Write-Host $line -ForegroundColor Green }
            'Failed'    { Write-Host $line -ForegroundColor Red }
            'Warning'   { Write-Host $line -ForegroundColor Yellow }
            'Skipped'   { Write-Host $line -ForegroundColor Cyan }
            default     { Write-Host $line }
        }
    }
}

$repoRoot     = $PSScriptRoot | Split-Path -Parent
$solutionPath = Join-Path $repoRoot 'MatrixRain.sln'

$toolsScript = Join-Path $PSScriptRoot 'VSTools.ps1'
if (-not (Test-Path $toolsScript)) {
    throw "Tool helper script not found: $toolsScript"
}

. $toolsScript

if (-not (Test-Path $solutionPath)) {
    throw "Solution not found: $solutionPath"
}

$msbuildPath = Get-VS2026MSBuildPath
if (-not $msbuildPath) {
    $msbuildPath = Get-VS2026MSBuildPath -IncludePrerelease
}

if (-not $msbuildPath) {
    throw 'VS 2026 (v18.x) MSBuild not found (via vswhere). Install VS 2026 with MSBuild.'
}





function Test-BuildArtifacts {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Platform,
        [Parameter(Mandatory = $true)]
        [string]$Configuration
    )

    $outputDir = Join-Path -Path $repoRoot -ChildPath "$Platform\$Configuration"
    $exePath   = Join-Path -Path $outputDir -ChildPath 'MatrixRain.exe'
    $scrPath   = Join-Path -Path $outputDir -ChildPath 'MatrixRain.scr'

    Write-Host "Verifying build artifacts..." -ForegroundColor Cyan

    if (-not (Test-Path -Path $exePath)) {
        Write-Error "Build artifact missing: $exePath"
        return $false
    }
    Write-Host "  [OK] $exePath" -ForegroundColor Green

    if (-not (Test-Path -Path $scrPath)) {
        Write-Error "Build artifact missing: $scrPath"
        return $false
    }
    Write-Host "  [OK] $scrPath" -ForegroundColor Green

    return $true
}





$scriptExitCode = 0

try {
    if ($Target -eq 'BuildAllRelease' -or $Target -eq 'RebuildAllRelease' -or $Target -eq 'CleanAll') {
        $platformsToBuild = @('x64', 'ARM64')
        if (-not (Test-VSVCPlatformInstalled -MSBuildPath $msbuildPath -Platform 'ARM64')) {
            Write-Host 'ARM64 C++ build targets not installed; skipping ARM64.' -ForegroundColor Cyan
            $msbuildTarget = if ($Target -eq 'CleanAll') { 'Clean' } elseif ($Target -eq 'RebuildAllRelease') { 'Rebuild' } else { 'Build' }
            Add-BuildResult -Configuration 'Release' -Platform 'ARM64' -Target $msbuildTarget -Status 'Skipped' -Message 'ARM64 C++ build tools not installed'
            $platformsToBuild = @('x64')
        }

        # For CleanAll, clean both Debug and Release for all platforms
        if ($Target -eq 'CleanAll') {
            $configsToBuild = @('Debug', 'Release')
            $msbuildTarget = 'Clean'
        }
        else {
            $configsToBuild = @('Release')
            $msbuildTarget = if ($Target -eq 'RebuildAllRelease') { 'Rebuild' } else { 'Build' }
        }

        foreach ($configToBuild in $configsToBuild) {
            foreach ($platformToBuild in $platformsToBuild) {
                $msbuildArgs = @(
                    $solutionPath,
                    "-p:Configuration=$configToBuild",
                    "-p:Platform=$platformToBuild",
                    "-t:$msbuildTarget"
                )

                # Use native ARM64 compiler when building ARM64 on ARM64 host
                if ($platformToBuild -eq 'ARM64' -and [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq [System.Runtime.InteropServices.Architecture]::Arm64) {
                    $msbuildArgs += "-p:PreferredToolArchitecture=arm64"
                }

                Write-Host "Using MSBuild: $msbuildPath"
                Write-Host "Building: $solutionPath ($configToBuild|$platformToBuild) Target=$msbuildTarget"

                $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
                & $msbuildPath @msbuildArgs
                $stopwatch.Stop()

                if ($LASTEXITCODE -ne 0) {
                    Add-BuildResult -Configuration $configToBuild -Platform $platformToBuild -Target $msbuildTarget -Status 'Failed' -ExitCode $LASTEXITCODE -Duration $stopwatch.Elapsed
                    $scriptExitCode = $LASTEXITCODE
                    break
                }

                # Verify artifacts for Build/Rebuild targets (not Clean)
                if ($msbuildTarget -ne 'Clean') {
                    if (-not (Test-BuildArtifacts -Platform $platformToBuild -Configuration $configToBuild)) {
                        Add-BuildResult -Configuration $configToBuild -Platform $platformToBuild -Target $msbuildTarget -Status 'Failed' -ExitCode 1 -Duration $stopwatch.Elapsed -Message 'Build artifacts missing'
                        $scriptExitCode = 1
                        break
                    }
                }

                Add-BuildResult -Configuration $configToBuild -Platform $platformToBuild -Target $msbuildTarget -Status 'Succeeded' -Duration $stopwatch.Elapsed
            }

            if ($scriptExitCode -ne 0) {
                break
            }
        }

        if ($scriptExitCode -ne 0) {
            foreach ($configToBuild in $configsToBuild) {
                foreach ($platformToBuild in $platformsToBuild) {
                    $existing = $script:BuildResults | Where-Object { $_.Configuration -eq $configToBuild -and $_.Platform -eq $platformToBuild -and $_.Target -eq $msbuildTarget } | Select-Object -First 1
                    if (-not $existing) {
                        Add-BuildResult -Configuration $configToBuild -Platform $platformToBuild -Target $msbuildTarget -Status 'Skipped' -Message 'Skipped due to previous failure'
                    }
                }
            }
        }
    }
    else {
        if ($Platform -eq 'ARM64') {
            if (-not (Test-VSVCPlatformInstalled -MSBuildPath $msbuildPath -Platform 'ARM64')) {
                Add-BuildResult -Configuration $Configuration -Platform $Platform -Target $Target -Status 'Failed' -ExitCode 1 -Message 'ARM64 C++ build tools not installed'
                throw 'ARM64 C++ build targets are not installed in this Visual Studio instance. Install the MSVC ARM64 build tools (Desktop development with C++), or build x64 instead.'
            }
        }

        $msbuildArgs = @(
            $solutionPath,
            "-p:Configuration=$Configuration",
            "-p:Platform=$Platform"
        )

        # Use native ARM64 compiler when building ARM64 on ARM64 host
        if ($Platform -eq 'ARM64' -and [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq [System.Runtime.InteropServices.Architecture]::Arm64) {
            $msbuildArgs += "-p:PreferredToolArchitecture=arm64"
        }

        if ($Target -ne 'Build') {
            $msbuildArgs += "-t:$Target"
        }

        Write-Host "Using MSBuild: $msbuildPath"
        Write-Host "Building: $solutionPath ($Configuration|$Platform) Target=$Target"

        $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        & $msbuildPath @msbuildArgs
        $stopwatch.Stop()

        if ($LASTEXITCODE -ne 0) {
            Add-BuildResult -Configuration $Configuration -Platform $Platform -Target $Target -Status 'Failed' -ExitCode $LASTEXITCODE -Duration $stopwatch.Elapsed
            $scriptExitCode = $LASTEXITCODE
        }
        else {
            # Verify build artifacts for Build/Rebuild targets
            if ($Target -eq 'Build' -or $Target -eq 'Rebuild') {
                if (-not (Test-BuildArtifacts -Platform $Platform -Configuration $Configuration)) {
                    Add-BuildResult -Configuration $Configuration -Platform $Platform -Target $Target -Status 'Failed' -ExitCode 1 -Duration $stopwatch.Elapsed -Message 'Build artifacts missing'
                    $scriptExitCode = 1
                }
                else {
                    Add-BuildResult -Configuration $Configuration -Platform $Platform -Target $Target -Status 'Succeeded' -Duration $stopwatch.Elapsed
                }
            }
            else {
                Add-BuildResult -Configuration $Configuration -Platform $Platform -Target $Target -Status 'Succeeded' -Duration $stopwatch.Elapsed
            }
        }
    }
}
catch {
    if ($scriptExitCode -eq 0) {
        $scriptExitCode = 1
    }

    Write-Host $_ -ForegroundColor Red
}
finally {
    Write-BuildSummary
}

exit $scriptExitCode
