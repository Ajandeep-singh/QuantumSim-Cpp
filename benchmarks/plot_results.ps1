param(
    [string]$InputPath = "results/benchmark_raw.csv",
    [string]$OutputDirectory = "results/figures"
)

$ErrorActionPreference = "Stop"
$culture = [System.Globalization.CultureInfo]::InvariantCulture
$data = @(Import-Csv -LiteralPath $InputPath)
if ($data.Count -eq 0) { throw "No benchmark rows found in $InputPath" }
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

function F([double]$value) { return $value.ToString("0.###", $culture) }

function Median($values) {
    $sorted = @($values | Sort-Object)
    $middle = [math]::Floor($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) { return [double]$sorted[$middle] }
    return ([double]$sorted[$middle - 1] + [double]$sorted[$middle]) / 2.0
}

function New-LineChart([string]$Title, [string]$Subtitle, [string]$YAxis,
    [object[]]$XValues, [object[]]$Series, [string]$Path,
    [double]$MinExponent, [double]$MaxExponent) {
    $titleXml = [System.Security.SecurityElement]::Escape($Title)
    $subtitleXml = [System.Security.SecurityElement]::Escape($Subtitle)
    $yAxisXml = [System.Security.SecurityElement]::Escape($YAxis)
    $width = 900; $height = 520
    $left = 95.0; $right = 840.0; $top = 88.0; $bottom = 420.0
    $plotWidth = $right - $left; $plotHeight = $bottom - $top
    $parts = [System.Collections.Generic.List[string]]::new()
    $parts.Add("<svg xmlns='http://www.w3.org/2000/svg' width='$width' height='$height' viewBox='0 0 $width $height'>")
    $parts.Add("<rect width='100%' height='100%' fill='white'/>")
    $parts.Add("<text x='$left' y='34' font-family='Arial' font-size='22' font-weight='bold'>$titleXml</text>")
    $parts.Add("<text x='$left' y='59' font-family='Arial' font-size='13' fill='#444'>$subtitleXml</text>")
    for ($tick = [math]::Ceiling($MinExponent); $tick -le [math]::Floor($MaxExponent); $tick++) {
        $y = $bottom - (($tick - $MinExponent) / ($MaxExponent - $MinExponent)) * $plotHeight
        $parts.Add("<line x1='$(F $left)' y1='$(F $y)' x2='$(F $right)' y2='$(F $y)' stroke='#dddddd'/>")
        $label = "10^$tick"
        $parts.Add("<text x='$(F ($left - 12))' y='$(F ($y + 4))' text-anchor='end' font-family='Arial' font-size='12'>$label</text>")
    }
    $parts.Add("<line x1='$(F $left)' y1='$(F $top)' x2='$(F $left)' y2='$(F $bottom)' stroke='#222'/>")
    $parts.Add("<line x1='$(F $left)' y1='$(F $bottom)' x2='$(F $right)' y2='$(F $bottom)' stroke='#222'/>")
    for ($i = 0; $i -lt $XValues.Count; $i++) {
        $x = if ($XValues.Count -eq 1) { $left + $plotWidth / 2 } else { $left + $i * $plotWidth / ($XValues.Count - 1) }
        $parts.Add("<text x='$(F $x)' y='$(F ($bottom + 23))' text-anchor='middle' font-family='Arial' font-size='12'>$($XValues[$i])</text>")
    }
    $parts.Add("<text x='$(F (($left + $right) / 2))' y='$(F ($height - 30))' text-anchor='middle' font-family='Arial' font-size='14'>Category</text>")
    $parts.Add("<text transform='translate(22 $(F (($top + $bottom) / 2))) rotate(-90)' text-anchor='middle' font-family='Arial' font-size='14'>$yAxisXml (log scale)</text>")
    $colors = @('#1769aa', '#d1495b', '#2a9d8f', '#e09f3e', '#6a4c93', '#457b9d')
    for ($s = 0; $s -lt $Series.Count; $s++) {
        $seriesNameXml = [System.Security.SecurityElement]::Escape($Series[$s].Name)
        $points = [System.Collections.Generic.List[string]]::new()
        for ($i = 0; $i -lt $Series[$s].Values.Count; $i++) {
            $value = [double]$Series[$s].Values[$i]
            if ($value -le 0) { $value = [math]::Pow(10.0, $MinExponent) }
            $x = if ($XValues.Count -eq 1) { $left + $plotWidth / 2 } else { $left + $i * $plotWidth / ($XValues.Count - 1) }
            $exp = [math]::Log10($value)
            $exp = [math]::Min($MaxExponent, [math]::Max($MinExponent, $exp))
            $y = $bottom - (($exp - $MinExponent) / ($MaxExponent - $MinExponent)) * $plotHeight
            $points.Add("$(F $x),$(F $y)")
            $parts.Add("<circle cx='$(F $x)' cy='$(F $y)' r='3.5' fill='$($colors[$s % $colors.Count])'/>")
        }
        $parts.Add("<polyline fill='none' stroke='$($colors[$s % $colors.Count])' stroke-width='2' points='$($points -join ' ' )'/>")
        $legendY = 470 + [math]::Floor($s / 3) * 20
        $legendX = 115 + ($s % 3) * 255
        $parts.Add("<line x1='$legendX' y1='$legendY' x2='$($legendX + 24)' y2='$legendY' stroke='$($colors[$s % $colors.Count])' stroke-width='3'/>")
        $parts.Add("<text x='$($legendX + 30)' y='$(F ($legendY + 4))' font-family='Arial' font-size='12'>$seriesNameXml</text>")
    }
    $parts.Add('</svg>')
    [System.IO.File]::WriteAllText($Path, ($parts -join "`n"), [System.Text.Encoding]::UTF8)
}

$runtimeAtDepth = @($data | Where-Object { [int]$_.depth -eq 100 })
$qubits = @($runtimeAtDepth | Select-Object -ExpandProperty qubits -Unique | Sort-Object {[int]$_} | ForEach-Object {[int]$_})
$series = @()
foreach ($operation in @('X', 'P')) {
    foreach ($implementation in @('generic_matrix', 'specialized')) {
        $values = foreach ($q in $qubits) {
            $group = @($runtimeAtDepth | Where-Object { $_.operation -eq $operation -and $_.implementation -eq $implementation -and [int]$_.qubits -eq $q })
            (Median @($group | ForEach-Object {[double]$_.elapsed_ns_per_circuit}))
        }
        $series += [pscustomobject]@{Name="$operation $implementation"; Values=@($values)}
    }
}
New-LineChart "Gate runtime versus qubit count" "Median of 3 runs; circuit depth 100; high-index target qubit" "Elapsed time (ns)" $qubits $series (Join-Path $OutputDirectory 'runtime_by_qubits.svg') 2 9

$depthRows = @($data | Where-Object { [int]$_.qubits -eq 14 })
$depths = @($depthRows | Select-Object -ExpandProperty depth -Unique | Sort-Object {[int]$_} | ForEach-Object {[int]$_})
$series = @()
foreach ($operation in @('X', 'P')) {
    foreach ($implementation in @('generic_matrix', 'specialized')) {
        $values = foreach ($depth in $depths) {
            $group = @($depthRows | Where-Object { $_.operation -eq $operation -and $_.implementation -eq $implementation -and [int]$_.depth -eq $depth })
            (Median @($group | ForEach-Object {[double]$_.elapsed_ns_per_circuit}))
        }
        $series += [pscustomobject]@{Name="$operation $implementation"; Values=@($values)}
    }
}
New-LineChart "Gate runtime versus repeated gate depth" "Median of 3 runs; 14 qubits; target qubit 13" "Elapsed time (ns)" $depths $series (Join-Path $OutputDirectory 'runtime_by_depth.svg') 2 9

$memoryRows = @($data | Where-Object { [int]$_.depth -eq 1 -and $_.operation -eq 'X' -and $_.implementation -eq 'generic_matrix' -and [int]$_.repeat -eq 1 })
$memoryQuBits = @($memoryRows | ForEach-Object {[int]$_.qubits} | Sort-Object)
$memoryValues = @($memoryRows | ForEach-Object {[double]$_.state_bytes})
$series = @([pscustomobject]@{Name='Measured amplitude vector capacity'; Values=$memoryValues})
New-LineChart "State-vector memory versus qubit count" "Measured vector capacity; sizeof(complex<double>) x capacity" "Bytes" $memoryQuBits $series (Join-Path $OutputDirectory 'memory_by_qubits.svg') 2 7

$errorSeries = @()
foreach ($operation in @('X', 'P')) {
    $values = foreach ($depth in $depths) {
        $group = @($data | Where-Object { $_.operation -eq $operation -and [int]$_.depth -eq $depth })
        $maximum = ($group | ForEach-Object {[double]$_.norm_error} | Measure-Object -Maximum).Maximum
        if ($maximum -le 0) { $maximum = 1e-18 }
        $maximum
    }
    $errorSeries += [pscustomobject]@{Name="$operation max norm error"; Values=@($values)}
}
New-LineChart "Normalization error versus gate depth" "Maximum absolute norm drift across all measured qubit counts and implementations" "Absolute error" $depths $errorSeries (Join-Path $OutputDirectory 'normalization_error.svg') -18 -10

Write-Output "Created plots in $OutputDirectory"
