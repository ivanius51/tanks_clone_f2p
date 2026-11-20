<#
Packs dist\Release into a zip archive for players: dist\TanksClone-<version>-win32.zip.

Usage:
  tools\make_release.ps1                 # zips what the last Release build put into dist\Release
  tools\make_release.ps1 -Build          # builds the Release configuration first (needs MSBuild in PATH)
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
	Get-Process TanksClone -ErrorAction SilentlyContinue | Stop-Process -Force
	MSBuild (Join-Path $root 'proj.win32\TanksClone.sln') /p:Configuration=Release /p:Platform=Win32 `
		/p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0.22621.0 /m
	if ($LASTEXITCODE -ne 0) { throw "the Release build failed" }
}

if (-not (Test-Path (Join-Path $release 'TanksClone.exe'))) {
	throw "dist\Release\TanksClone.exe not found: build the Release configuration first (or pass -Build)"
}

$archive = Join-Path $root "dist\TanksClone-$Version-win32.zip"
if (Test-Path $archive) { Remove-Item $archive -Force }
Compress-Archive -Path (Join-Path $release '*') -DestinationPath $archive
Write-Host "release archive: $archive"
