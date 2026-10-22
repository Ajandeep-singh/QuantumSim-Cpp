param(
    [string]$InputPath = "report/QuantumSim_Research_Report.md",
    [string]$OutputPath = "report/QuantumSim_Research_Report.html"
)

$ErrorActionPreference = "Stop"
$lines = @(Get-Content -LiteralPath $InputPath -Encoding UTF8)
$body = [System.Collections.Generic.List[string]]::new()
$inCode = $false
$inList = $false
$inTable = $false
$inParagraph = $false

function Close-Paragraph { if ($script:inParagraph) { $script:body.Add('</p>'); $script:inParagraph = $false } }
function Close-List { if ($script:inList) { $script:body.Add('</ul>'); $script:inList = $false } }
function Close-Table { if ($script:inTable) { $script:body.Add('</tbody></table>'); $script:inTable = $false } }
function Inline([string]$text) {
    $encoded = [System.Net.WebUtility]::HtmlEncode($text)
    $encoded = [regex]::Replace($encoded, '\*\*(.+?)\*\*', '<strong>$1</strong>')
    $encoded = [regex]::Replace($encoded, '`([^`]+)`', '<code>$1</code>')
    $encoded = [regex]::Replace($encoded, '\[([^\]]+)\]\(([^)]+)\)', '<a href="$2">$1</a>')
    $encoded = [regex]::Replace($encoded, '(?<!["=])(https?://[^\s<]+)', '<a href="$1">$1</a>')
    return $encoded
}

foreach ($line in $lines) {
    if ($line.StartsWith('```')) {
        Close-Paragraph; Close-List; Close-Table
        if ($inCode) { $body.Add('</code></pre>'); $inCode = $false }
        else { $body.Add('<pre><code>'); $inCode = $true }
        continue
    }
    if ($inCode) {
        $body.Add(([System.Net.WebUtility]::HtmlEncode($line) + "`n"))
        continue
    }
    if ([string]::IsNullOrWhiteSpace($line)) {
        Close-Paragraph; Close-List; Close-Table
        continue
    }
    if ($line -match '^\|') {
        Close-Paragraph; Close-List
        $cells = @($line.Trim('|').Split('|') | ForEach-Object {$_.Trim()})
        if ($line -match '^\|?\s*:?-{2,}') { continue }
        if (-not $inTable) {
            $body.Add('<table><thead><tr>')
            foreach ($cell in $cells) { $body.Add("<th>$(Inline $cell)</th>") }
            $body.Add('</tr></thead><tbody>')
            $inTable = $true
        } else {
            $body.Add('<tr>')
            foreach ($cell in $cells) { $body.Add("<td>$(Inline $cell)</td>") }
            $body.Add('</tr>')
        }
        continue
    }
    Close-Table
    if ($line -match '^!\[([^\]]+)\]\(([^)]+)\)$') {
        Close-Paragraph; Close-List
        $alt = [System.Net.WebUtility]::HtmlEncode($Matches[1])
        $src = [System.Net.WebUtility]::HtmlEncode($Matches[2])
        $body.Add("<figure><img src='$src' alt='$alt'><figcaption>$alt</figcaption></figure>")
        continue
    }
    if ($line -match '^(#{1,3})\s+(.+)$') {
        Close-Paragraph; Close-List
        $level = $Matches[1].Length
        $heading = Inline $Matches[2]
        $body.Add("<h$level>$heading</h$level>")
        continue
    }
    if ($line -match '^[-*]\s+(.+)$') {
        Close-Paragraph
        if (-not $inList) { $body.Add('<ul>'); $inList = $true }
        $body.Add("<li>$(Inline $Matches[1])</li>")
        continue
    }
    Close-List
    if (-not $inParagraph) { $body.Add('<p>'); $inParagraph = $true }
    else { $body.Add(' ') }
    $body.Add((Inline $line.TrimEnd()))
}
Close-Paragraph; Close-List; Close-Table
if ($inCode) { $body.Add('</code></pre>') }

$html = @"
<!doctype html>
<html lang="en"><head><meta charset="utf-8"><title>QuantumSim-C++ Research Report</title>
<style>
@page { size: A4; margin: 18mm 17mm 19mm; }
body { font-family: Arial, Helvetica, sans-serif; color: #20242a; font-size: 10.5pt; line-height: 1.48; max-width: 900px; margin: 2em auto; }
h1 { font-size: 25pt; line-height: 1.15; border-bottom: 2px solid #1769aa; padding-bottom: .35em; }
h2 { font-size: 17pt; color: #174a70; margin-top: 1.7em; page-break-after: avoid; }
h3 { font-size: 13pt; page-break-after: avoid; }
p { margin: .4em 0 .8em; } table { width: 100%; border-collapse: collapse; margin: 1em 0; font-size: 9.5pt; }
th,td { border: 1px solid #c7d0d8; padding: 6px 8px; text-align: left; } th { background: #eaf2f8; }
code,pre { font-family: Consolas, monospace; } code { background: #f2f4f6; padding: 1px 3px; }
pre { white-space: pre-wrap; background: #f3f5f7; border-left: 3px solid #1769aa; padding: 10px; font-size: 8.5pt; }
figure { margin: 1.2em 0; page-break-inside: avoid; text-align: center; } figure img { max-width: 100%; height: auto; } figcaption { font-size: 9pt; color: #5c6670; }
li { margin: .2em 0; } a { color: #145f91; overflow-wrap: anywhere; }
@media print { body { max-width: none; margin: 0; } h1 { page-break-before: avoid; } }
</style></head><body>
$($body -join "`n")
</body></html>
"@
[System.IO.File]::WriteAllText((Join-Path (Get-Location) $OutputPath), $html, [System.Text.Encoding]::UTF8)
Write-Output "Wrote $OutputPath"
