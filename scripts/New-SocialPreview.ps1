<#
.SYNOPSIS
    Generates the GitHub social preview card (assets/social-preview.png).

.DESCRIPTION
    Composites a real MatrixRain frame with the wordmark, accent rule, tagline
    and a scanline pass (the app's own 1.5 effect). Output is 1280x640, the size
    GitHub asks for under Settings > General > Social preview.

.EXAMPLE
    pwsh -File scripts/New-SocialPreview.ps1
#>
[CmdletBinding()]
param(
    [string] $Background = (Join-Path $PSScriptRoot '..\assets\MatrixRain-social.jpg'),
    [string] $OutputPath = (Join-Path $PSScriptRoot '..\assets\social-preview.png')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing

$W = 1280
$H = 640
$X = 72

function New-Argb {
    param([int] $A, [int] $R, [int] $G, [int] $B)
    [System.Drawing.Color]::FromArgb($A, $R, $G, $B)
}

# Draws text with a soft green bloom underneath, mimicking the app's glow pass.
function Write-GlowText {
    param(
        [System.Drawing.Graphics] $Graphics,
        [string] $Text,
        [System.Drawing.Font] $Font,
        [System.Drawing.Color] $Color,
        [single] $X,
        [single] $Y,
        [int] $Radius = 8,
        [int] $Alpha = 26
    )

    $glow = New-Argb $Alpha 0 230 118
    $glowBrush = New-Object System.Drawing.SolidBrush $glow
    for ($r = $Radius; $r -ge 2; $r -= 2) {
        foreach ($angle in 0, 45, 90, 135, 180, 225, 270, 315) {
            $rad = $angle * [Math]::PI / 180.0
            $dx = [single]($r * [Math]::Cos($rad))
            $dy = [single]($r * [Math]::Sin($rad))
            $Graphics.DrawString($Text, $Font, $glowBrush, $X + $dx, $Y + $dy)
        }
    }
    $glowBrush.Dispose()

    $brush = New-Object System.Drawing.SolidBrush $Color
    $Graphics.DrawString($Text, $Font, $brush, $X, $Y)
    $brush.Dispose()
}

$bgPath = (Resolve-Path $Background).Path
$src = [System.Drawing.Image]::FromFile($bgPath)
$bmp = New-Object System.Drawing.Bitmap $W, $H
$g = [System.Drawing.Graphics]::FromImage($bmp)
try {
    $g.InterpolationMode = 'HighQualityBicubic'
    $g.SmoothingMode = 'AntiAlias'
    $g.TextRenderingHint = 'AntiAliasGridFit'

    # Background: cover-crop a real frame of the rain.
    $scale = [Math]::Max($W / $src.Width, $H / $src.Height)
    $dw = [single]($src.Width * $scale)
    $dh = [single]($src.Height * $scale)
    $g.DrawImage($src, [single](($W - $dw) / 2), [single](($H - $dh) / 2), $dw, $dh)

    # Left scrim, so the wordmark reads while the rain stays visible behind it.
    $rect = New-Object System.Drawing.Rectangle 0, 0, ([int]($W * 0.78)), $H
    $scrim = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        $rect, (New-Argb 250 1 6 3), (New-Argb 0 1 6 3), 0.0)
    $blend = New-Object System.Drawing.Drawing2D.ColorBlend 4
    $blend.Colors = @((New-Argb 250 1 6 3), (New-Argb 240 1 6 3), (New-Argb 150 1 6 3), (New-Argb 0 1 6 3))
    $blend.Positions = @(0.0, 0.45, 0.72, 1.0)
    $scrim.InterpolationColors = $blend
    $g.FillRectangle($scrim, $rect)
    $scrim.Dispose()

    # Top/bottom vignette.
    $full = New-Object System.Drawing.Rectangle 0, 0, $W, $H
    $vign = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        $full, (New-Argb 140 0 0 0), (New-Argb 165 0 0 0), 90.0)
    $vblend = New-Object System.Drawing.Drawing2D.ColorBlend 4
    $vblend.Colors = @((New-Argb 140 0 0 0), (New-Argb 0 0 0 0), (New-Argb 0 0 0 0), (New-Argb 165 0 0 0))
    $vblend.Positions = @(0.0, 0.35, 0.72, 1.0)
    $vign.InterpolationColors = $vblend
    $g.FillRectangle($vign, $full)
    $vign.Dispose()

    $titleFont = New-Object System.Drawing.Font 'Segoe UI', 68, ([System.Drawing.FontStyle]::Bold), ([System.Drawing.GraphicsUnit]::Pixel)
    $bodyFont = New-Object System.Drawing.Font 'Segoe UI', 28, ([System.Drawing.FontStyle]::Regular), ([System.Drawing.GraphicsUnit]::Pixel)
    $quipFont = New-Object System.Drawing.Font 'Segoe UI', 22, ([System.Drawing.FontStyle]::Regular), ([System.Drawing.GraphicsUnit]::Pixel)
    $monoFont = New-Object System.Drawing.Font 'Consolas', 19, ([System.Drawing.FontStyle]::Regular), ([System.Drawing.GraphicsUnit]::Pixel)

    # Wordmark: white leader glyph over a green trail, the rain's own palette.
    Write-GlowText $g 'MatrixRain' $titleFont (New-Argb 255 234 255 241) ($X - 6) 214 10 30

    # Accent rule.
    $ruleGlow = New-Object System.Drawing.SolidBrush (New-Argb 60 0 230 118)
    $g.FillRectangle($ruleGlow, $X - 4, 322, 112, 14)
    $ruleGlow.Dispose()
    $rule = New-Object System.Drawing.SolidBrush (New-Argb 255 0 230 118)
    $g.FillRectangle($rule, $X, 326, 104, 6)
    $rule.Dispose()

    $body = New-Object System.Drawing.SolidBrush (New-Argb 255 215 255 229)
    $g.DrawString('Matrix rain screensaver for Windows.', $bodyFont, $body, ($X - 4), 372)
    $g.DrawString('Win32 + DirectX, every monitor, 60 fps.', $bodyFont, $body, ($X - 4), 412)
    $body.Dispose()

    $quip = New-Object System.Drawing.SolidBrush (New-Argb 245 122 180 145)
    $g.DrawString('There is no spoon. There is a screensaver.', $quipFont, $quip, ($X - 4), 470)
    $quip.Dispose()

    # In-world flourish: where /install actually puts it.
    $installPath = 'C:\Windows\System32\MatrixRain.scr'
    $size = $g.MeasureString($installPath, $monoFont)
    # A dark plate keeps the path legible over the busiest part of the rain.
    $plate = New-Object System.Drawing.SolidBrush (New-Argb 170 1 6 3)
    $g.FillRectangle($plate, [single]($W - 52 - $size.Width), [single]($H - 56),
        [single]($size.Width + 24), [single]($size.Height + 16))
    $plate.Dispose()
    Write-GlowText $g $installPath $monoFont (New-Argb 255 156 255 196) `
        ([single]($W - 40 - $size.Width)) ([single]($H - 48)) 4 22

    # Signature scanline pass, kept subtle so it survives GitHub's resizing.
    $scan = New-Object System.Drawing.SolidBrush (New-Argb 46 0 0 0)
    for ($y = 0; $y -lt $H; $y += 3) { $g.FillRectangle($scan, 0, $y, $W, 1) }
    $scan.Dispose()

    foreach ($f in $titleFont, $bodyFont, $quipFont, $monoFont) { $f.Dispose() }

    $outFull = [IO.Path]::GetFullPath($OutputPath)
    $bmp.Save($outFull, [System.Drawing.Imaging.ImageFormat]::Png)
    Write-Host "Wrote $outFull ($W x $H)"
}
finally {
    $g.Dispose()
    $bmp.Dispose()
    $src.Dispose()
}
