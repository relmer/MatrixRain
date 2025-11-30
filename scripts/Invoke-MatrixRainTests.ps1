[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64', 'Win32')]
    [string]$Platform = 'x64',

    [string]$RunSettings = 'MatrixRainTests.runsettings',

    [switch]$Parallel
)

$ErrorActionPreference = 'Stop'

function Get-VisualStudioInstallationPath
{
    $programFilesX86 = [Environment]::GetEnvironmentVariable('ProgramFiles(x86)')
    if ([string]::IsNullOrWhiteSpace($programFilesX86))
    {
        throw 'ProgramFiles(x86) environment variable not found.'
    }

    $vswherePath = Join-Path -Path $programFilesX86 -ChildPath 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -Path $vswherePath))
    {
        throw "vswhere.exe not found at the expected location: $vswherePath"
    }

    $installationPath = & $vswherePath -latest -requires Microsoft.Component.MSBuild -property installationPath
    if ([string]::IsNullOrWhiteSpace($installationPath))
    {
        # Fall back to any installed product in case the MSBuild workload name shifts.
        $installationPath = & $vswherePath -latest -products * -property installationPath
    }

    if ([string]::IsNullOrWhiteSpace($installationPath))
    {
        throw 'No Visual Studio installation detected.'
    }

    return $installationPath
}

function Get-VsTestConsolePath
{
    param([string]$InstallationPath)

    $candidatePaths = @(
        Join-Path -Path $InstallationPath -ChildPath 'Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe'
        Join-Path -Path $InstallationPath -ChildPath 'Common7\IDE\Extensions\TestPlatform\vstest.console.exe'
    )

    foreach ($candidate in $candidatePaths)
    {
        if (Test-Path -Path $candidate)
        {
            return $candidate
        }
    }

    $searchRoot = Join-Path -Path $InstallationPath -ChildPath 'Common7\IDE'
    $found = Get-ChildItem -Path $searchRoot -Filter 'vstest.console.exe' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -ne $found)
    {
        return $found.FullName
    }

    throw 'vstest.console.exe not found in the detected Visual Studio installation.'
}

$repositoryRoot = Resolve-Path -Path (Join-Path -Path $PSScriptRoot -ChildPath '..')
Push-Location -Path $repositoryRoot

try
{
    $vsInstallPath = Get-VisualStudioInstallationPath
    $vstestPath    = Get-VsTestConsolePath -InstallationPath $vsInstallPath

    $testAssembly = Join-Path -Path $repositoryRoot -ChildPath "x64\\$Configuration\\MatrixRainTests.dll"
    if (-not (Test-Path -Path $testAssembly))
    {
        throw "Test assembly not found at $testAssembly. Build the tests before running them."
    }

    $runSettingsPath = Join-Path -Path $repositoryRoot -ChildPath $RunSettings
    if (-not (Test-Path -Path $runSettingsPath))
    {
        throw "Runsettings file not found at $runSettingsPath."
    }

    $vstestArguments = @($testAssembly, "/Settings:$runSettingsPath")
    if ($Parallel.IsPresent)
    {
        $vstestArguments += '/Parallel'
    }

    Write-Host "Running tests from $testAssembly" -ForegroundColor Cyan
    Write-Host "vstest.console path: $vstestPath" -ForegroundColor DarkGray

    & $vstestPath @vstestArguments
    if ($LASTEXITCODE -ne 0)
    {
        exit $LASTEXITCODE
    }
}
finally
{
    Pop-Location
}
