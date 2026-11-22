# HAPPY JARZ 0.18.15 - publish ONLY the exact Board #3 tested candidate.
# PowerShell 5.1-compatible ASCII source. Does not build, flash or erase hardware.
param([switch]$Push)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Set-Location $root

$branch = (& git branch --show-current).Trim()
if ($LASTEXITCODE -ne 0 -or $branch -ne 'feature/mini-art-gallery-animation') {
  throw "STOP: Wrong branch '$branch'."
}
$version = (Get-Content 'firmware/VERSION' -Raw).Trim()
if ($version -ne '0.18.15') { throw "STOP: This publisher only accepts 0.18.15." }
$expected = '877a228da3cb2fc627fa84032d8773dd9db4e4904db550534527bb34b2626762'
$size = 1119025
$source = Join-Path $root 'release-staging\candidate\happyjarz-0.18.15-app.bin'
$entryPath = Join-Path $root 'release-staging\candidate\release-entry.json'
if (!(Test-Path $source) -or !(Test-Path $entryPath)) { throw 'STOP: Tested candidate or manifest missing.' }
$actual = (Get-FileHash $source -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -ne $expected) { throw "STOP: Binary hash differs from tested Board #3: $actual." }
if ((Get-Item $source).Length -ne $size) { throw 'STOP: Unexpected app size.' }
$entry = Get-Content $entryPath -Raw | ConvertFrom-Json
if ($entry.version -ne $version -or $entry.channel -ne 'stable' -or
    $entry.model -ne 'happyjarz-supermini-simple-4mb' -or $entry.hw -ne 'V1' -or
    $entry.chip -ne 'ESP32-S3' -or
    $entry.layout -ne 'arduino-4mb-default-app-0x10000-0x140000' -or
    $entry.flashSize -ne 4194304 -or $entry.images.Count -ne 1 -or
    $entry.images[0].address -ne 65536 -or $entry.images[0].size -ne $size -or
    $entry.images[0].sha256 -ne $actual -or
    $entry.images[0].url -ne './releases/happyjarz-0.18.15-app.bin') {
  throw 'STOP: Candidate release metadata mismatch.'
}

$catalogPath = Join-Path $root 'web-controller\releases.json'
$catalog = Get-Content $catalogPath -Raw | ConvertFrom-Json
if ($catalog.schema -ne 1 -or $catalog.channel -ne 'stable') {
  throw 'STOP: Unexpected catalog schema/channel.'
}
$prior = @($catalog.releases | Where-Object { $_.version -eq '0.18.14' })
$others = @($catalog.releases | Where-Object { $_.version -ne '0.18.14' -and $_.version -ne '0.18.15' })
if ($prior.Count -ne 1 -or $others.Count -ne 0) {
  throw 'STOP: Expected original 0.18.14 release only.'
}
$baseline = $prior[0]
if ($baseline.hw -ne 'V1' -or $baseline.images.Count -ne 1 -or
    $baseline.images[0].sha256 -ne 'a92108a7acd697e761c9859d0a8b77f5a6eef2526fd19ff872b10fa251ddb712') {
  throw 'STOP: Existing 0.18.14 baseline changed unexpectedly.'
}

$target = Join-Path $root 'web-controller\releases\happyjarz-0.18.15-app.bin'
if (Test-Path $target) {
  if ((Get-FileHash $target -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) {
    throw 'STOP: Existing target release differs.'
  }
} else {
  Copy-Item $source $target
}
$catalog.status = 'published'
$catalog.releases = @($entry, $baseline)
$json = $catalog | ConvertTo-Json -Depth 12
[System.IO.File]::WriteAllText($catalogPath, $json + [Environment]::NewLine, (New-Object System.Text.UTF8Encoding($false)))
if ((Get-FileHash $target -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) { throw 'STOP: Published image checksum failure.' }

Write-Host "STAGED 0.18.15 - $size bytes - SHA-256 $expected" -ForegroundColor Green
Write-Host 'Preserved 0.18.14 as prior release.'
if ($Push) {
  & git add -- 'web-controller/releases.json' 'web-controller/releases/happyjarz-0.18.15-app.bin'
  if ($LASTEXITCODE -ne 0) { throw 'git add failed.' }
  & git commit -m 'Publish Board 3 tested HAPPY JARZ 0.18.15'
  if ($LASTEXITCODE -ne 0) { throw 'git commit failed. Check git status.' }
  & git push origin feature/mini-art-gallery-animation
  if ($LASTEXITCODE -ne 0) { throw 'git push failed. Site not published.' }
  Write-Host 'Release pushed. Verify the live site deployment before attempting updates.' -ForegroundColor Cyan
} else {
  Write-Host 'Local stage only. Run with -Push to publish the tested build.' -ForegroundColor Yellow
}
