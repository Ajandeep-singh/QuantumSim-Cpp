param(
    [string]$HtmlPath = "report/QuantumSim_Research_Report.html",
    [string]$OutputPath = "report/QuantumSim_Research_Report.pdf"
)

$ErrorActionPreference = "Stop"
$chromeCandidates = @(
    (Join-Path $env:ProgramFiles "Google/Chrome/Application/chrome.exe"),
    (Join-Path $env:ProgramFiles "BraveSoftware/Brave-Browser/Application/brave.exe")
)
$chrome = $chromeCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $chrome) { throw "Install Chrome or Brave to export the HTML report as PDF." }
$htmlFull = (Resolve-Path -LiteralPath $HtmlPath).Path
$pdfFull = [System.IO.Path]::GetFullPath($OutputPath)
$htmlUri = ([System.Uri]::new($htmlFull)).AbsoluteUri
$arguments = @(
    "--headless=new",
    "--disable-gpu",
    "--allow-file-access-from-files",
    "--no-first-run",
    "--no-default-browser-check",
    "--disable-extensions",
    ('--print-to-pdf="' + $pdfFull + '"'),
    ('"' + $htmlUri + '"')
)
$process = Start-Process -FilePath $chrome -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
if ($process.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $pdfFull)) {
    throw "Browser PDF export failed."
}
Write-Output "Wrote $pdfFull"
