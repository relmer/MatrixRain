<#
.SYNOPSIS
    Renders the GitHub social preview card (assets/social-preview.png).

.DESCRIPTION
    The card itself is drawn by scripts/social-preview.html, which composites a
    real MatrixRain frame with the wordmark, accent rule, tagline and a scanline
    pass (the app's own 1.5 effect). This script just screenshots that page with
    headless Edge at 1280x640, the size GitHub asks for under
    Settings > General > Social preview.

    Edge does the rendering because the card leans on canvas shadow blur for the
    glow. Reimplementing that in GDI+ is not worth the drift.

    Edit the HTML to change the card; this script needs no changes.

.EXAMPLE
    pwsh -File scripts/New-SocialPreview.ps1
#>
[CmdletBinding()]
param(
    [string] $Page = (Join-Path $PSScriptRoot 'social-preview.html'),
    [string] $OutputPath = (Join-Path $PSScriptRoot '..\assets\social-preview.png'),
    [string] $EdgePath,

    # Quality of the downscaled background handed to the page. Lower blooms the
    # rain more; the card was designed around 72.
    [ValidateRange(1, 100)]
    [int] $BackgroundQuality = 72
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $EdgePath) {
    $candidates = @(
        "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe",
        "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe"
    )
    $EdgePath = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $EdgePath) {
    throw 'Could not find msedge.exe. Pass -EdgePath explicitly.'
}

$pageFull = (Resolve-Path $Page).Path
$outFull = [IO.Path]::GetFullPath($OutputPath)

# The rain's bloom is a compression artefact, and a wanted one: downscaling the
# still and running it through JPEG smears the saturated green sideways and rings
# around the bright glyphs. Chromium downscaling the PNG itself looks far crisper.
# The prepared file sits beside the page so it can be referenced relatively;
# a file:// URL in the query string is blocked as a cross-origin image load.
Add-Type -AssemblyName System.Drawing
$bgSource = (Resolve-Path (Join-Path $PSScriptRoot '..\assets\MatrixRain.png')).Path
$bgPrepared = Join-Path (Split-Path $pageFull) '_background.jpg'

$still = [System.Drawing.Image]::FromFile($bgSource)
try {
    $scaled = New-Object System.Drawing.Bitmap 1280, 853
    $sg = [System.Drawing.Graphics]::FromImage($scaled)
    $sg.InterpolationMode = 'HighQualityBicubic'
    $sg.DrawImage($still, 0, 0, 1280, 853)
    $sg.Dispose()

    $jpeg = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() |
        Where-Object { $_.MimeType -eq 'image/jpeg' }
    $encParams = New-Object System.Drawing.Imaging.EncoderParameters 1
    $encParams.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter(
        [System.Drawing.Imaging.Encoder]::Quality, [long] $BackgroundQuality)
    $scaled.Save($bgPrepared, $jpeg, $encParams)
    $encParams.Dispose()
    $scaled.Dispose()
}
finally {
    $still.Dispose()
}

$url = 'file:///' + ($pageFull -replace '\\', '/') + '?bg=_background.jpg'

# A throwaway profile keeps this off the user's real Edge profile.
$profileDir = Join-Path ([IO.Path]::GetTempPath()) "matrixrain-preview-$PID"

try {
    # --virtual-time-budget lets the background image decode and the canvas
    # finish drawing before the screenshot is taken.
    & $EdgePath --headless=new --disable-gpu --no-sandbox --hide-scrollbars `
        --virtual-time-budget=5000 --force-device-scale-factor=1 `
        --window-size=1280,640 --screenshot=$outFull `
        --user-data-dir=$profileDir $url 2>&1 | Out-Null

    # Edge can return before the screenshot is flushed to disk.
    $deadline = (Get-Date).AddSeconds(20)
    while (-not (Test-Path $outFull) -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 250
    }
    if (-not (Test-Path $outFull)) {
        throw "Edge produced no screenshot at $outFull."
    }

    $kb = [int]((Get-Item $outFull).Length / 1KB)
    Write-Host "Wrote $outFull (1280 x 640, $kb KB)"
    if ($kb -gt 1024) {
        Write-Warning "Over 1 MB: GitHub's social preview upload will reject this file."
    }
}
finally {
    Remove-Item $profileDir -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item $bgPrepared -Force -ErrorAction SilentlyContinue
}
