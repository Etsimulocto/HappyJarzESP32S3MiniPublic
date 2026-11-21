# HAPPY JARZ — Windows firmware release candidate builder
# Compiles and stages a SHA-256-verified APP-ONLY candidate.
# NEVER flashes hardware or publishes releases.json.
param(
    [string]$Version = "",
    [switch]$SkipDependencyInstall
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$sketch = Join-Path $root "firmware\HappyJarzMini"
$build = Join-Path $root "release-staging\build"
$out = Join-Path $root "release-staging\candidate"
$recorded = (Get-Content (Join-Path $root "firmware\VERSION") -Raw).Trim()
if (-not $Version) { $Version = $recorded }
if ($Version -ne $recorded) { throw "Requested $Version but firmware/VERSION is $recorded" }

$ino = Get-Content (Join-Path $sketch "HappyJarzMini.ino") -Raw
if (-not $ino.Contains("HJ_FW_VERSION = `"$Version`"")) {
    throw "Firmware source and VERSION differ. Refusing to package."
}

if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
    throw "arduino-cli not installed or not in PATH. Install Arduino CLI first."
}
if (-not (Get-Command py -ErrorAction SilentlyContinue)) {
    throw "Windows Python launcher 'py' not found. Install Python 3 first."
}

New-Item -ItemType Directory -Force -Path $build, $out | Out-Null
if (-not $SkipDependencyInstall) {
    & arduino-cli core update-index
    if ($LASTEXITCODE -ne 0) { throw "Arduino board index update failed" }
    & arduino-cli core install esp32:esp32
    if ($LASTEXITCODE -ne 0) { throw "ESP32 core install failed" }
    & arduino-cli lib install U8g2
    if ($LASTEXITCODE -ne 0) { throw "U8g2 install failed" }
}

# Explicit 4MB SuperMini and native USB CDC. No PSRAM assumptions.
$fqbn = "esp32:esp32:esp32s3:FlashSize=4M,CDCOnBoot=cdc"
Write-Host "Building candidate $Version with $fqbn" -ForegroundColor Cyan
& arduino-cli compile --fqbn $fqbn --output-dir $build $sketch
if ($LASTEXITCODE -ne 0) { throw "Compilation failed. No release staged." }

$app = @(Get-ChildItem -Path $build -File -Filter "*.bin" | Where-Object {
    $_.Name -match "^HappyJarzMini(\.ino)?\.bin$"
})
$partitions = @(Get-ChildItem -Path $build -File -Filter "*.partitions.bin")
if ($app.Count -ne 1 -or $partitions.Count -ne 1) {
    Write-Host "Build output:"; Get-ChildItem $build | Select-Object Name, Length
    throw "Cannot uniquely identify application and partition binaries. No release staged."
}

# This verifies the COMPILED layout, not yet the partition table currently
# installed on Board #3. Do not publish without comparing to real hardware.
& py -3 (Join-Path $root "tools\stage_web_release.py") `
    --app $app[0].FullName `
    --partitions $partitions[0].FullName `
    --version $Version `
    --verified-layout `
    --out $out
if ($LASTEXITCODE -ne 0) { throw "Partition/checksum verification failed" }

Write-Host ""
Write-Host "CANDIDATE STAGED — NOT PUBLISHED OR FLASHED" -ForegroundColor Yellow
Write-Host "App binary: $out\happyjarz-$Version-app.bin"
Write-Host "Manifest:   $out\release-entry.json"
Write-Host "Next: verify Board #3 partition layout, back up settings/art,"
Write-Host "flash via Arduino CLI for first test, then test website installation."
