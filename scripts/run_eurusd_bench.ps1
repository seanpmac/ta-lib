param(
    [string]$DataDir = "C:\GitHub\QuantConnect\Lean\Data\forex\oanda\second\eurusd",
    [int]$Iterations = 5,
    [string[]]$Indicators = @("HT_SINE", "STOCH"),
    [string]$Mode = "serial"
)

$repoRoot = Split-Path -Parent $PSScriptRoot
$taPerfPath = Join-Path $repoRoot "build-win\bin\Release\ta_perf.exe"
if (-not (Test-Path $taPerfPath)) {
    throw "ta_perf.exe not found at $taPerfPath. Build ta_perf before running benchmarks."
}

$timestamp = Get-Date -Format 'yyyyMMddTHHmmssZ'
$runDir = Join-Path $repoRoot "benchmarks"
$runDir = Join-Path $runDir $timestamp
$runDir = Join-Path $runDir "eurusd"
New-Item -Path $runDir -ItemType Directory -Force | Out-Null

$indicatorList = $Indicators -join ','

Get-ChildItem -Path $DataDir -Filter '*.csv' | ForEach-Object {
    $outputPath = Join-Path $runDir ($_.BaseName + '.json')
    & $taPerfPath --input $_.FullName --mode $Mode --indicators $indicatorList --iterations $Iterations --output $outputPath
}

Write-Output "Benchmark reports written to $runDir"
