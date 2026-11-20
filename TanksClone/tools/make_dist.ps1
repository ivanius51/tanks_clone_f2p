<#
Assembles a runnable copy of the game after a build (called by the post-build step of proj.win32).

  1. Mirrors Resources\ to <BinDir>\Resources so the game runs from the build folder.
  2. Builds dist\<Configuration>\ with only what the game needs:
       TanksClone.exe, the engine DLLs, Resources\
       Debug:   + TanksClone.pdb (tests: TanksClone.exe --test)
       Test:    Release optimizations with the debug tools: + TanksClone.pdb, + the Visual C++ runtime DLLs
       Release: + the Visual C++ runtime DLLs (app-local), no symbols, no tests

The Test build links the Release engine: -EngineDir points at its folder, and its DLLs are copied next to the exe.

Usage: tools\make_dist.ps1 -Configuration Debug|Test|Release -BinDir proj.win32\Debug.win32 [-EngineDir proj.win32\Release.win32]
#>
param(
	[Parameter(Mandatory = $true)][ValidateSet('Debug', 'Test', 'Release')][string]$Configuration,
	[Parameter(Mandatory = $true)][string]$BinDir,
	[string]$EngineDir
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$resources = Join-Path $root 'Resources'
$binDir = (Resolve-Path $BinDir).Path
$dist = Join-Path $root "dist\$Configuration"
$exe = 'TanksClone.exe'

function Sync-Folder($source, $target) {
	# robocopy: exit codes below 8 mean success.
	robocopy $source $target /MIR /NJH /NJS /NFL /NDL /NP | Out-Null
	if ($LASTEXITCODE -ge 8) { throw "robocopy $source -> $target failed with code $LASTEXITCODE" }
}

function Find-VcRuntime {
	$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
	if (-not (Test-Path $vswhere)) { return @() }
	$install = & $vswhere -latest -products * -property installationPath
	if (-not $install) { return @() }
	$crt = Get-ChildItem (Join-Path $install 'VC\Redist\MSVC') -Directory -ErrorAction SilentlyContinue |
		Sort-Object Name -Descending |
		ForEach-Object { Get-ChildItem (Join-Path $_.FullName 'x86') -Directory -Filter 'Microsoft.VC*.CRT' -ErrorAction SilentlyContinue } |
		Select-Object -First 1
	if (-not $crt) { return @() }
	return Get-ChildItem $crt.FullName -Filter *.dll
}

if ($EngineDir) {
	Get-ChildItem (Resolve-Path $EngineDir).Path -Filter *.dll | ForEach-Object { Copy-Item $_.FullName $binDir -Force }
}

Sync-Folder $resources (Join-Path $binDir 'Resources')

New-Item -ItemType Directory -Force $dist | Out-Null
Sync-Folder $resources (Join-Path $dist 'Resources')

$wanted = @($exe) + (Get-ChildItem $binDir -Filter *.dll | ForEach-Object { $_.Name })
if ($Configuration -ne 'Release') { $wanted += 'TanksClone.pdb' }

foreach ($name in $wanted) {
	$source = Join-Path $binDir $name
	if (Test-Path $source) { Copy-Item $source $dist -Force }
}
if ($Configuration -ne 'Debug') {
	foreach ($dll in Find-VcRuntime) {
		Copy-Item $dll.FullName $dist -Force
		$wanted += $dll.Name
	}
}

Get-ChildItem $dist -File | Where-Object { $wanted -notcontains $_.Name } | Remove-Item -Force
Write-Host "dist\$Configuration is ready"
exit 0
