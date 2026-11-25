<#
Builds the Cocos2d-x engine once and keeps the result in libs\<Debug|Release>\ (libs, DLLs, engine PDB).
The game project only links these binaries and never builds the engine; the Test configuration uses the Release set.

The set is rebuilt only when engine_patches\ or the engine version changed (or with -Force). A stamp file in
libs\<Config>\ holds what the set was built from.

Usage:
  tools\engine_libs.ps1                      # Debug and Release, only what is out of date
  tools\engine_libs.ps1 -Configuration Release
  tools\engine_libs.ps1 -Force               # rebuild the engine
  tools\engine_libs.ps1 -Check               # exit code 1 when a set is missing or out of date, builds nothing
#>
param(
	[ValidateSet('Debug', 'Release', 'All')][string]$Configuration = 'All',
	[switch]$Force,
	[switch]$Check
)

. "$PSScriptRoot\common.ps1"

$engineRoot = Join-Path $script:Root 'cocos2d'
$libsRoot = Join-Path $script:Root 'libs'
$solutionDir = Join-Path $script:Root 'proj.win32\'
$engineProjects = @(
	(Join-Path $engineRoot 'cocos\editor-support\spine\proj.win32\libSpine.vcxproj'),
	(Join-Path $engineRoot 'cocos\2d\libcocos2d.vcxproj')
)
$StampName = 'engine.stamp'

function Get-EngineStamp {
	$version = (Get-Content (Join-Path $engineRoot 'CHANGELOG') -TotalCount 1)
	$patches = Get-ChildItem (Join-Path $script:Root 'engine_patches') -Recurse -File | Sort-Object FullName |
		ForEach-Object { (Get-FileHash $_.FullName -Algorithm SHA256).Hash }
	return ($version + "`n" + ($patches -join "`n"))
}

function Test-LibsUpToDate([string]$Config, [string]$Stamp) {
	$stampFile = Join-Path $libsRoot "$Config\$StampName"
	if (-not (Test-Path $stampFile)) { return $false }
	if (-not (Test-Path (Join-Path $libsRoot "$Config\libcocos2d.lib"))) { return $false }
	return ((Get-Content $stampFile -Raw).TrimEnd() -eq $Stamp)
}

$stamp = Get-EngineStamp
$configurations = if ($Configuration -eq 'All') { @('Debug', 'Release') } else { @($Configuration) }
$msbuild = $null
$failed = $false

foreach ($config in $configurations) {
	if (-not $Force -and (Test-LibsUpToDate $config $stamp)) {
		Write-Ok "libs\$config is up to date"
		continue
	}
	if ($Check) {
		Write-Bad "libs\$config is missing or out of date: run tools\engine_libs.ps1"
		$failed = $true
		continue
	}

	if (-not $msbuild) { $msbuild = Find-MSBuild }
	Stop-Game
	foreach ($project in $engineProjects) {
		$arguments = @($project, '/t:Build', "/p:Configuration=$config", '/p:Platform=Win32', "/p:SolutionDir=$solutionDir",
			'/p:PlatformToolset=v143', '/p:WindowsTargetPlatformVersion=10.0.22621.0', '/m', '/nologo', '/v:m')
		Write-Step "Engine $config : $(Split-Path -Leaf $project)"
		$result = Invoke-Logged -Title "Engine $config" -File $msbuild -Arguments $arguments -LogName "engine_${config}_$([IO.Path]::GetFileNameWithoutExtension($project))"
		if ($result.ExitCode -ne 0) {
			Write-Bad "Engine $config FAILED, log: $($result.Log)"
			Select-String -Path $result.Log -Pattern ': (fatal )?error ' | Select-Object -First 10 | ForEach-Object { Write-Host ('  ' + $_.Line.Trim()) -ForegroundColor Yellow }
			throw "engine build failed: $config"
		}
	}

	$source = Join-Path $script:Root "proj.win32\$config.win32"
	$target = Join-Path $libsRoot $config
	New-Item -ItemType Directory -Force $target | Out-Null
	Get-ChildItem $source -File | Where-Object {
		($_.Extension -in '.lib', '.dll') -and $_.BaseName -ne 'TanksClone'
	} | ForEach-Object { Copy-Item $_.FullName $target -Force }
	$enginePdb = Join-Path $source 'libcocos2d.pdb'
	if (Test-Path $enginePdb) { Copy-Item $enginePdb $target -Force }
	Set-Content -Path (Join-Path $target $StampName) -Value $stamp
	Write-Ok "libs\$config ready"
}

if ($failed) { exit 1 }
