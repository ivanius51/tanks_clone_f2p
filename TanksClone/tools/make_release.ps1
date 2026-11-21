<#
Packs dist\Release into a zip archive for players: dist\TanksClone-<version>-win32.zip.

Usage:
  tools\make_release.ps1                 # zips what the last Release build put into dist\Release
  tools\make_release.ps1 -Build          # builds the Release configuration first (finds MSBuild itself, see build.ps1)
  tools\make_release.ps1 -Version 0.2.0
#>
param(
	[string]$Version = (Get-Date -Format 'yyyy.MM.dd'),
	[switch]$Build
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$release = Join-Path $root 'dist\Release'

if ($Build) {
	& (Join-Path $PSScriptRoot 'build.ps1') -Configuration Release
}

if (-not (Test-Path (Join-Path $release 'TanksClone.exe'))) {
	throw "dist\Release\TanksClone.exe not found: build the Release configuration first (or pass -Build)"
}

$archive = Join-Path $root "dist\TanksClone-$Version-win32.zip"
if (Test-Path $archive) { Remove-Item $archive -Force }
Compress-Archive -Path (Join-Path $release '*') -DestinationPath $archive
Write-Host "release archive: $archive"
