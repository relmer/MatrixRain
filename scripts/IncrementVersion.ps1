# IncrementVersion.ps1
# Automatically increments the build number and updates the year in Version.h before each build

$repoRoot = Split-Path $PSScriptRoot -Parent
$versionFile = "$repoRoot\MatrixRainCore\Version.h"
$tempFile = "$versionFile.tmp"
$backupFile = "$versionFile.bak"

# Check if the version file exists
if (-not (Test-Path $versionFile)) {
    Write-Error "Version file not found: $versionFile"
    exit 1
}

# Read the file content
try {
    $content = Get-Content $versionFile -Raw -ErrorAction Stop
} catch {
    Write-Error "Failed to read $versionFile : $_"
    exit 1
}

# Validate the file has expected content before proceeding
if ([string]::IsNullOrWhiteSpace($content) -or $content -notmatch '#define VERSION_BUILD \d+') {
    Write-Warning "Version.h appears corrupted or empty. Attempting to restore from git..."

    try {
        Push-Location $repoRoot
        git checkout HEAD -- "MatrixRainCore/Version.h" 2>&1 | Out-Null
        Pop-Location

        # Re-read after restore
        $content = Get-Content $versionFile -Raw -ErrorAction Stop

        if ($content -notmatch '#define VERSION_BUILD \d+') {
            Write-Error "Failed to restore Version.h from git"
            exit 1
        }
        Write-Host "Successfully restored Version.h from git" -ForegroundColor Yellow
    } catch {
        Write-Error "Failed to restore Version.h: $_"
        exit 1
    }
}

# Get current year
$currentYear = (Get-Date).Year

# Find and increment the build number
if ($content -match '#define VERSION_BUILD (\d+)') {
    $buildNumber = [int]$matches[1] + 1
    $newContent = $content -replace '#define VERSION_BUILD \d+', "#define VERSION_BUILD $buildNumber"

    # Update the year (only report if it actually changed)
    $yearChanged = $false
    if ($content -match '#define VERSION_YEAR (\d+)') {
        $yearChanged = ([int]$matches[1] -ne $currentYear)
    }
    $newContent = $newContent -replace '#define VERSION_YEAR \d+', "#define VERSION_YEAR $currentYear"

    # Write to temp file first (atomic write pattern)
    try {
        Set-Content -Path $tempFile -Value $newContent -NoNewline -ErrorAction Stop
    } catch {
        Write-Error "Failed to write temp file: $_"
        if (Test-Path $tempFile) { Remove-Item $tempFile -Force -ErrorAction SilentlyContinue }
        exit 1
    }

    # Verify temp file was written correctly
    $verifyContent = Get-Content $tempFile -Raw -ErrorAction SilentlyContinue
    if ($verifyContent -notmatch "#define VERSION_BUILD $buildNumber") {
        Write-Error "Temp file verification failed"
        Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
        exit 1
    }

    # Backup original, replace with new (with retry for locked files)
    $maxRetries = 3
    $retryDelay = 500  # milliseconds

    for ($i = 0; $i -lt $maxRetries; $i++) {
        try {
            Copy-Item $versionFile $backupFile -Force -ErrorAction Stop
            Move-Item $tempFile $versionFile -Force -ErrorAction Stop
            Remove-Item $backupFile -Force -ErrorAction SilentlyContinue

            Write-Host "Build number incremented to: $buildNumber" -ForegroundColor Green
            if ($yearChanged) {
                Write-Host "Year updated to: $currentYear" -ForegroundColor Green
            }
            exit 0
        } catch {
            Write-Warning "Attempt $($i + 1) failed: $_"
            Start-Sleep -Milliseconds $retryDelay
        }
    }

    # All retries failed - restore from backup if it exists
    Write-Error "Failed to update Version.h after $maxRetries attempts"
    if (Test-Path $backupFile) {
        Copy-Item $backupFile $versionFile -Force -ErrorAction SilentlyContinue
        Remove-Item $backupFile -Force -ErrorAction SilentlyContinue
    }
    Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
    exit 1
} else {
    Write-Error "Could not find VERSION_BUILD in $versionFile"
    exit 1
}
