[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64', 'ARM64', 'Auto')]
    [string]$Platform = 'Auto',

    [string]$RunSettings = 'MatrixRainTests.runsettings',

    [switch]$Parallel
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

$repoRoot = $PSScriptRoot | Split-Path -Parent

$toolsScript = Join-Path $PSScriptRoot 'VSTools.ps1'
if (-not (Test-Path $toolsScript)) {
    throw "Tool helper script not found: $toolsScript"
}

. $toolsScript

# Always use native test runner for current OS architecture (not test platform)
$vstestPath = Get-VS2026VSTestPath
if (-not $vstestPath) {
    $vstestPath = Get-VS2026VSTestPath -IncludePrerelease
}

if (-not $vstestPath) {
    throw 'vstest.console.exe not found in VS 2026 (v18.x). Install VS 2026 with Test workload.'
}

$testAssembly = Join-Path -Path $repoRoot -ChildPath "$Platform\$Configuration\MatrixRainTests.dll"
if (-not (Test-Path -Path $testAssembly)) {
    throw "Test assembly not found at $testAssembly. Build the tests before running them."
}

$runSettingsPath = Join-Path -Path $repoRoot -ChildPath $RunSettings
if (-not (Test-Path -Path $runSettingsPath)) {
    throw "Runsettings file not found at $runSettingsPath."
}

$vstestArguments = @($testAssembly, "/Settings:$runSettingsPath")
if ($Parallel.IsPresent) {
    $vstestArguments += '/Parallel'
}

Write-Host "Running tests from $testAssembly" -ForegroundColor Cyan
Write-Host "vstest.console path: $vstestPath" -ForegroundColor DarkGray

& $vstestPath @vstestArguments
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
