# Run ONLY on the Windows PC with Board #3 connected by USB.
# Builds a LOCAL test website containing one firmware candidate.
# Does NOT publish anything to GitHub or modify the production site.
param(
    [Parameter(Mandatory=$true)][string]$Port,
    [switch]$IUnderstandFlashRisk,
    [switch]$IHaveBackedUpData
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not $IUnderstandFlashRisk -or -not $IHaveBackedUpData) {
    throw "Requires -IUnderstandFlashRisk AND -IHaveBackedUpData. Verify backups before enabling a local firmware flash."
}
if ($Port -notmatch '^COM[0-9]+$') { throw "Pass Board #3's actual COM port, e.g. -Port COM4" }
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$version = (Get-Content (Join-Path $root 'firmware\VERSION') -Raw).Trim()
$candidate = Join-Path $root 'release-staging\candidate'
$app = Join-Path $candidate "happyjarz-$version-app.bin"
$built = Join-Path $candidate 'verified-partitions.bin'
$entry = Join-Path $candidate 'release-entry.json'
$site = Join-Path $root 'release-staging\local-site'
$boardTable = Join-Path $root 'release-staging\board3-partitions.bin'
foreach($path in @($app,$built,$entry)) {
    if(-not (Test-Path $path)) { throw "Missing candidate artifact: $path. Run build_release_windows.ps1 first." }
}
if(-not (Get-Command py -ErrorAction SilentlyContinue)) { throw 'Python 3 launcher not found.' }
Write-Host 'CLOSE Edge jar connection, Pi app, serial monitors and Arduino Serial Monitor before reading flash.' -ForegroundColor Yellow
Write-Host "Reading current partition layout from Board #3 on $Port (read only)..." -ForegroundColor Cyan
& py -3 -m esptool --chip esp32s3 --port $Port read-flash 0x8000 0x1000 $boardTable
if($LASTEXITCODE -ne 0) { throw 'Could not read board partition table. No test website enabled.' }
& py -3 (Join-Path $root 'tools\verify_board_partitions.py') --built $built --board $boardTable
if($LASTEXITCODE -ne 0) { throw 'Board layout mismatch. No test website enabled.' }

New-Item -ItemType Directory -Force -Path $site | Out-Null
Copy-Item -Path (Join-Path $root 'web-controller\*') -Destination $site -Recurse -Force
$releases = Join-Path $site 'releases'
New-Item -ItemType Directory -Force -Path $releases | Out-Null
Copy-Item -Path $app -Destination (Join-Path $releases (Split-Path $app -Leaf)) -Force
$releaseEntry = Get-Content $entry -Raw | ConvertFrom-Json
@{schema=1; releases=@($releaseEntry)} |
    ConvertTo-Json -Depth 12 | Set-Content -Path (Join-Path $site 'releases.json') -Encoding UTF8
Write-Host ''
Write-Host 'LOCAL TEST SITE READY — NOT PUBLICLY RELEASED' -ForegroundColor Green
Write-Host "Firmware candidate: $version"
Write-Host 'Open http://localhost:8000 in desktop Edge, connect Board #3, open UPDATES.'
Write-Host 'Keep USB attached, use explicit confirmation. Use this test board only.'
Write-Host 'Ctrl+C stops the temporary website server.'
& py -3 -m http.server 8000 --bind 127.0.0.1 --directory $site
