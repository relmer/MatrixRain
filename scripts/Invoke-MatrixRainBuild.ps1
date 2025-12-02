[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Build', 'Clean', 'Rebuild')]
    [string]$Target,

    [Parameter(Mandatory = $true)]
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration,

    [ValidateSet('x64', 'Win32')]
    [string]$Platform = 'x64'
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
        throw 'No Visual Studio instance with MSBuild found.'
    }

    return $installationPath
}

function Get-MsBuildPath
{
    param([string]$InstallationPath)

    $msbuildPath = Join-Path -Path $InstallationPath -ChildPath 'MSBuild\Current\Bin\MSBuild.exe'
    if (-not (Test-Path -Path $msbuildPath))
    {
        throw "MSBuild.exe not found at $msbuildPath"
    }

    return $msbuildPath
}

$repositoryRoot = Resolve-Path -Path (Join-Path -Path $PSScriptRoot -ChildPath '..')
Push-Location -Path $repositoryRoot

try
{
    $vsInstallPath = Get-VisualStudioInstallationPath
    $msbuildPath   = Get-MsBuildPath -InstallationPath $vsInstallPath

    $msbuildArguments = @(
        'MatrixRain.sln'
        "/t:$Target"
        "/p:Configuration=$Configuration"
        "/p:Platform=$Platform"
    )

    Write-Host "Running MSBuild ($Target $Configuration|$Platform)" -ForegroundColor Cyan
    Write-Host "MSBuild path: $msbuildPath" -ForegroundColor DarkGray

    & $msbuildPath @msbuildArguments
    if ($LASTEXITCODE -ne 0)
    {
        exit $LASTEXITCODE
    }

    # T009: Verify build artifacts exist (both .exe and .scr)
    if ($Target -eq 'Build' -or $Target -eq 'Rebuild')
    {
        $outputDir      = Join-Path -Path $repositoryRoot -ChildPath "$Platform\$Configuration"
        $exePath        = Join-Path -Path $outputDir -ChildPath 'MatrixRain.exe'
        $scrPath        = Join-Path -Path $outputDir -ChildPath 'MatrixRain.scr'

        Write-Host "Verifying build artifacts..." -ForegroundColor Cyan

        if (-not (Test-Path -Path $exePath))
        {
            Write-Error "Build artifact missing: $exePath"
            exit 1
        }
        Write-Host "  [OK] $exePath" -ForegroundColor Green

        if (-not (Test-Path -Path $scrPath))
        {
            Write-Error "Build artifact missing: $scrPath"
            exit 1
        }
        
        Write-Host "  [OK] $scrPath" -ForegroundColor Green
    }
}
finally
{
    Pop-Location
}
