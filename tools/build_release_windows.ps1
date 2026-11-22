# HAPPY JARZ - Windows firmware release candidate builder
# Compiles and stages a SHA-256-verified APP-ONLY candidate.
# NEVER flashes hardware or publishes releases.json.
param(
    [string]$Version = "",
    [switch]$InstallDependencies
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
if ($InstallDependencies) {
    Write-Warning 'Dependency installation may change your Arduino toolchain. Existing installs remain untouched without this switch.'
    & arduino-cli core update-index
    if ($LASTEXITCODE -ne 0) { throw "Arduino board index update failed" }
    & arduino-cli core install esp32:esp32
    if ($LASTEXITCODE -ne 0) { throw "ESP32 core install failed" }
    & arduino-cli lib install U8g2
    if ($LASTEXITCODE -ne 0) { throw "U8g2 install failed" }
}

# Explicit 4MB SuperMini, default dual-app/NVS/SPIFFS layout and native USB CDC.
# Existing core and libraries are reused unless -InstallDependencies was requested.
$fqbn = "esp32:esp32:esp32s3:FlashSize=4M,PartitionScheme=default,PSRAM=disabled,CDCOnBoot=cdc"
# Preserve a toolchain fingerprint with the staged package for reproduction.
& arduino-cli version | Out-File -FilePath (Join-Path $out 'toolchain-version.txt')
& arduino-cli core list | Out-File -FilePath (Join-Path $out 'toolchain-cores.txt')
& arduino-cli lib list | Out-File -FilePath (Join-Path $out 'toolchain-libraries.txt')
Write-Host "Building candidate $Version with $fqbn" -ForegroundColor Cyan
Write-Host "Progress is an activity heartbeat, NOT a percent estimate." -ForegroundColor DarkGray
$compileOut = Join-Path $out "compile-stdout.log"
$compileErr = Join-Path $out "compile-stderr.log"
# Keep the UI responsive even if Arduino CLI is silent while compiling.
$cliExe = (Get-Command arduino-cli -ErrorAction Stop).Source
$cliArgs = @("compile", "--fqbn", $fqbn, "--output-dir", $build, $sketch)
$buildTimer = [System.Diagnostics.Stopwatch]::StartNew()
$compiler = Start-Process -FilePath $cliExe -ArgumentList $cliArgs -PassThru -NoNewWindow -RedirectStandardOutput $compileOut -RedirectStandardError $compileErr
try {
    while (-not $compiler.HasExited) {
        Start-Sleep -Seconds 2
        $compiler.Refresh()
        $elapsed = $buildTimer.Elapsed.ToString("mm\:ss")
        Write-Host ("[BUILD] {0} elapsed - Arduino compiler active..." -f $elapsed) -ForegroundColor DarkCyan
    }
    $compiler.WaitForExit()
    $exitCode = $compiler.ExitCode
} finally {
    $buildTimer.Stop()
    $compiler.Dispose()
}
if (Test-Path $compileOut) { Get-Content $compileOut | ForEach-Object { Write-Host $_ } }
if (Test-Path $compileErr) { Get-Content $compileErr | ForEach-Object { Write-Host $_ -ForegroundColor Yellow } }
if ($exitCode -ne 0) {
    Write-Host "Compiler exit code: $exitCode" -ForegroundColor Red
    Write-Host "Diagnostics saved: $compileOut and $compileErr" -ForegroundColor Yellow
    throw "Compilation failed. No release staged."
}
Write-Host ("[BUILD] Compile completed in {0}." -f $buildTimer.Elapsed.ToString("mm\:ss")) -ForegroundColor Green

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
Write-Host "CANDIDATE STAGED - NOT PUBLISHED OR FLASHED" -ForegroundColor Yellow
Write-Host "App binary: $out\happyjarz-$Version-app.bin"
Write-Host "Manifest:   $out\release-entry.json"
Write-Host "Next: verify Board #3 partition layout, back up settings/art,"
Write-Host "flash via Arduino CLI for first test, then test website installation."
