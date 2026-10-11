param(
    [string]$InputPath = "results/benchmark_raw.csv",
    [string]$OutputPath = "results/benchmark_summary.csv"
)

$ErrorActionPreference = "Stop"
$rows = @(Import-Csv -LiteralPath $InputPath)
if ($rows.Count -eq 0) { throw "No benchmark rows found in $InputPath" }

$summary = foreach ($group in ($rows | Group-Object operation,implementation,qubits,depth)) {
    $ordered = @($group.Group | Sort-Object {[double]$_.elapsed_ns_per_circuit})
    $middle = [math]::Floor($ordered.Count / 2)
    $median = if (($ordered.Count % 2) -eq 1) {
        [double]$ordered[$middle].elapsed_ns_per_circuit
    } else {
        ([double]$ordered[$middle - 1].elapsed_ns_per_circuit + [double]$ordered[$middle].elapsed_ns_per_circuit) / 2.0
    }
    [pscustomobject]@{
        operation = $ordered[0].operation
        implementation = $ordered[0].implementation
        qubits = [int]$ordered[0].qubits
        depth = [int]$ordered[0].depth
        run_count = $ordered.Count
        median_ns_per_circuit = $median
        state_bytes = [long]$ordered[0].state_bytes
        max_norm_error = (($ordered | ForEach-Object {[double]$_.norm_error} | Measure-Object -Maximum).Maximum)
        max_reference_l2_error = (($ordered | ForEach-Object {[double]$_.reference_l2_error} | Measure-Object -Maximum).Maximum)
    }
}
$summary | Export-Csv -LiteralPath $OutputPath -NoTypeInformation
Write-Output "Wrote $($summary.Count) grouped rows to $OutputPath from $($rows.Count) raw measurements."
