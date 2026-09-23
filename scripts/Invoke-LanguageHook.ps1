<#
.SYNOPSIS
    Claude Code PreToolUse hook: blocks a Write or Edit whose new text uses
    British spelling or "name" as a verb.

.DESCRIPTION
    Reads the hook payload as JSON on stdin and checks only the text being
    ADDED -- `content` for a Write, `new_string` for an Edit -- so editing a
    file that already holds older British spellings does not trip on text the
    edit never touched.

    Exit code 2 blocks the tool call and feeds this script's stderr back as
    feedback, which is the whole point: the fix happens before the text ever
    reaches disk, rather than at commit time.

    Wire it up in .claude/settings.json under hooks.PreToolUse with the
    matcher "Write|Edit".
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'





$raw = [Console]::In.ReadToEnd()

if (-not $raw) {
    exit 0
}

try {
    $payload = $raw | ConvertFrom-Json
}
catch {
    # A payload this script cannot read must not block the edit; a language
    # check is not worth wedging the session over.
    exit 0
}

# The checker itself is exempt: its word list is, necessarily, a list of the
# very words it rejects.
if ($payload.tool_input.file_path -and
    $payload.tool_input.file_path -match 'Test-Language\.ps1$') {
    exit 0
}

$text = $null

if ($payload.tool_input.content) {
    $text = $payload.tool_input.content
}
elseif ($payload.tool_input.new_string) {
    $text = $payload.tool_input.new_string
}

if (-not $text) {
    exit 0
}

$checker = Join-Path $PSScriptRoot 'Test-Language.ps1'

if (-not (Test-Path $checker)) {
    exit 0
}

$report = & $checker -Text $text -Label 'new text' 2>&1

if ($LASTEXITCODE -eq 0) {
    exit 0
}

[Console]::Error.WriteLine(($report | Out-String))
[Console]::Error.WriteLine('Rewrite the offending text and try the edit again.')

exit 2
