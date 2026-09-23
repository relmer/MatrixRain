<#
.SYNOPSIS
    Checks text for British spelling and for "name" used as a verb.

.DESCRIPTION
    One checker, three callers: the git pre-commit and commit-msg hooks in
    .githooks, the Claude Code PreToolUse hook, and CI. Keeping the word list
    in a single place is the point -- adding a word here fixes every caller at
    once.

    It reports only what it is given. The git hook passes the ADDED lines of a
    commit, not whole files, so pre-existing text elsewhere in the repository
    does not fail a commit that never touched it. Cleaning that up is a
    separate decision.

.PARAMETER Path
    Files to check in full. Each reported violation carries its line number.

.PARAMETER Text
    A string to check instead of files, for a commit message or a hook payload.

.PARAMETER Label
    What to call the text in a report when -Text is used.

.EXAMPLE
    .\scripts\Test-Language.ps1 -Path src\Thing.cpp

.EXAMPLE
    git diff --cached -U0 | .\scripts\Test-Language.ps1 -Text (Get-Content -Raw)

.OUTPUTS
    Exit code 0 when clean, 1 when anything was found.
#>
[CmdletBinding()]
param(
    [string[]]$Path,

    [string]$Text,

    [string]$Label = 'input'
)

$ErrorActionPreference = 'Stop'





# British -> American. Order matters: longer forms must come before the
# shorter ones they contain, or "colours" reports as "colour".
$script:SpellingPairs = [ordered]@{
    'colours'        = 'colors'
    'colour'         = 'color'
    'behaviours'     = 'behaviors'
    'behaviour'      = 'behavior'
    'artefacts'      = 'artifacts'
    'artefact'       = 'artifact'
    'neighbours'     = 'neighbors'
    'neighbour'      = 'neighbor'
    'centred'        = 'centered'
    'centre'         = 'center'
    'initialisation' = 'initialization'
    'initialise'     = 'initialize'
    'recognise'      = 'recognize'
    'normalise'      = 'normalize'
    'optimise'       = 'optimize'
    'organise'       = 'organize'
    'summarise'      = 'summarize'
    'analyse'        = 'analyze'
    'apologise'      = 'apologize'
    'prioritise'     = 'prioritize'
    'quantise'       = 'quantize'
    'greyed'         = 'grayed'
    'grey'           = 'gray'
    'labelled'       = 'labeled'
    'cancelled'      = 'canceled'
    'modelled'       = 'modeled'
    'travelled'      = 'traveled'
    'whilst'         = 'while'
    'amongst'        = 'among'
    'towards'        = 'toward'
    'favour'         = 'favor'
    'honour'         = 'honor'
    'licence'        = 'license'
    'defence'        = 'defense'
    'practise'       = 'practice'
    'maths'          = 'math'
    'aluminium'      = 'aluminum'
}

$script:SpellingRegex = '\b(?:' + (($script:SpellingPairs.Keys) -join '|') + ')\b'

# "name" used as a verb. The noun is fine, so "its name is X", "the name of X"
# and identifiers such as GetName or a `name` parameter are all left alone.
$script:VerbPatterns = @(
    '\bnamed\b'
    '\bnaming\b'
    '\bnames\s+(?:the|a|an|it|each|one|what|which|its|their)\b'
)

$script:VerbAdvice = 'Use "name" only as a noun. Instead of the verb, say what the thing does: shows, gives, lists, says, holds, calls it, writes it, stands for. Where a noun is wanted, "title" often fits.'





function Get-LanguageViolation {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyString()][string]$Line,
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][int]$Number
    )

    $found = @()


    # One alternation, longest forms first, so "colours" wins over "colour" at
    # the same position, and every distinct hit on the line is reported rather
    # than only the first.
    foreach ($hit in [regex]::Matches($Line, $script:SpellingRegex, 'IgnoreCase')) {
        $british = $hit.Value.ToLowerInvariant()

        $found += [PSCustomObject]@{
            Source  = $Source
            Number  = $Number
            Kind    = 'spelling'
            Match   = $hit.Value
            Advice  = "British spelling: use '$($script:SpellingPairs[$british])'"
            Content = $Line.Trim()
        }
    }

    foreach ($pattern in $script:VerbPatterns) {
        if ($Line -imatch $pattern) {
            $found += [PSCustomObject]@{
                Source  = $Source
                Number  = $Number
                Kind    = 'name-as-verb'
                Match   = $Matches[0]
                Advice  = $script:VerbAdvice
                Content = $Line.Trim()
            }
            break
        }
    }

    return $found
}





$violations = @()

if ($Text) {
    $lines = $Text -split "`r?`n"

    for ($i = 0; $i -lt $lines.Count; $i++) {
        $violations += Get-LanguageViolation -Line $lines[$i] -Source $Label -Number ($i + 1)
    }
}

foreach ($file in $Path) {
    if (-not (Test-Path $file -PathType Leaf)) {
        continue
    }

    # @() matters: Get-Content hands back a bare string for a one-line file,
    # and indexing that walks characters instead of lines. A commit message is
    # very often one line.
    $lines = @(Get-Content -LiteralPath $file)

    for ($i = 0; $i -lt $lines.Count; $i++) {
        $violations += Get-LanguageViolation -Line $lines[$i] -Source $file -Number ($i + 1)
    }
}

if ($violations.Count -eq 0) {
    exit 0
}

Write-Host ''
Write-Host 'Language check failed.' -ForegroundColor Red
Write-Host ''

foreach ($v in $violations) {
    Write-Host ("  {0}:{1}  [{2}] '{3}'" -f $v.Source, $v.Number, $v.Kind, $v.Match) -ForegroundColor Yellow
    Write-Host ("      {0}" -f $v.Advice)
    Write-Host ("      > {0}" -f $v.Content) -ForegroundColor DarkGray
    Write-Host ''
}

Write-Host ("{0} violation(s). See the Language section of the global instructions." -f $violations.Count) -ForegroundColor Red
Write-Host ''

exit 1
