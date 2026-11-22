# Publish the *exact* physically tested HAPPY JARZ 0.18.14 app image.
# Windows PowerShell 5.1+. No flashing, rebuilding, or board access.
# Publishes to this git branch only; the public website must deploy that commit separately.
param([switch]$Push)
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $root
$branch = (& git branch --show-current).Trim()
if ($LASTEXITCODE -ne 0 -or $branch -ne 'feature/mini-art-gallery-animation') {
  throw "Stop: expected feature/mini-art-gallery-animation, found '$branch'."
}
$version = (Get-Content 'firmware/VERSION' -Raw).Trim()
if ($version -ne '0.18.14') { throw "This reviewed publisher only accepts firmware 0.18.14." }
$source = Join-Path $root "release-staging\candidate\happyjarz-0.18.14-app.bin"
$entryPath = Join-Path $root "release-staging\candidate\release-entry.json"
if (!(Test-Path $source) -or !(Test-Path $entryPath)) { throw "Missing tested candidate binary or release entry. Run the existing builder first, and test the result on hardware before publishing." }
$entry = Get-Content $entryPath -Raw | ConvertFrom-Json
$actual = (Get-FileHash $source -Algorithm SHA256).Hash.ToLowerInvariant()
$expected = 'a92108a7acd697e761c9859d0a8b77f5a6eef2526fd19ff872b10fa251ddb712'
if ($actual -ne $expected) { throw "STOP: app binary differs from the 0.18.14 build tested on Board #3 ($actual)." }
$bytes = (Get-Item $source).Length
if ($bytes -ne 1118144 -or $bytes -gt 0x140000) { throw "STOP: unexpected app image size $bytes." }
if ($entry.version -ne $version -or $entry.hw -ne 'V1' -or $entry.chip -ne 'ESP32-S3' -or
    $entry.layout -ne 'arduino-4mb-default-app-0x10000-0x140000' -or
    $entry.model -ne 'happyjarz-supermini-simple-4mb' -or
    $entry.images.Count -ne 1 -or $entry.images[0].address -ne 65536 -or
    $entry.images[0].size -ne $bytes -or $entry.images[0].sha256 -ne $actual -or
    $entry.images[0].url -ne './releases/happyjarz-0.18.14-app.bin') {
  throw 'STOP: release-entry metadata does not match verified candidate.'
}
$targetDir = Join-Path $root 'web-controller\releases'
New-Item -ItemType Directory -Force $targetDir | Out-Null
$target = Join-Path $targetDir 'happyjarz-0.18.14-app.bin'
if ((Test-Path $target) -and ((Get-FileHash $target -Algorithm SHA256).Hash.ToLowerInvariant() -ne $actual)) {
  throw 'STOP: existing release binary differs; refusing overwrite.'
}
Copy-Item $source $target -Force
$catalogPath = Join-Path $root 'web-controller\releases.json'
$catalog = Get-Content $catalogPath -Raw | ConvertFrom-Json
if ($catalog.schema -ne 1 -or $catalog.channel -ne 'stable' -or
    @($catalog.releases | Where-Object { $_.version -ne $version }).Count -gt 0) {
  throw 'STOP: unexpected release catalog. Review manually before replacing.'
}
$catalog.status = 'published'
$catalog.releases = @($entry)
$catalog | ConvertTo-Json -Depth 12 | Set-Content $catalogPath -Encoding UTF8
# PowerShell 5.1 writes a UTF-8 BOM; strip it for consistent static serving.
$json = [System.IO.File]::ReadAllText($catalogPath).TrimStart([char]0xFEFF)
[System.IO.File]::WriteAllText($catalogPath, $json + [Environment]::NewLine, (New-Object System.Text.UTF8Encoding($false)))
$published = (Get-FileHash $target -Algorithm SHA256).Hash.ToLowerInvariant()
if ($published -ne $expected) { throw 'STOP: copied file changed unexpectedly.' }
Write-Host "STAGED APPROVED RELEASE $version — $bytes bytes, SHA-256 $published" -ForegroundColor Green
Write-Host 'Git files: web-controller/releases.json and web-controller/releases/happyjarz-0.18.14-app.bin'
if ($Push) {
  & git add -- 'web-controller/releases.json' 'web-controller/releases/happyjarz-0.18.14-app.bin'
  if ($LASTEXITCODE -ne 0) { throw 'git add failed' }
  & git commit -m 'Publish hardware-tested HAPPY JARZ firmware 0.18.14'
  if ($LASTEXITCODE -ne 0) { throw 'git commit failed; inspect git status' }
  & git push origin feature/mini-art-gallery-animation
  if ($LASTEXITCODE -ne 0) { throw 'git push failed; the site is NOT published' }
  Write-Host 'Git branch pushed. Deploy the site from this branch before attempting web update.' -ForegroundColor Cyan
} else {
  Write-Host 'Not pushed. Re-run with -Push when ready.' -ForegroundColor Yellow
}
