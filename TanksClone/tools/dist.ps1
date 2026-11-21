<#
Shows what is in dist\ and packs a release archive.

Actions:
  Info      the size and build time of dist\Debug, dist\Test, dist\Release
  Release   builds the Release configuration and packs dist\Release into dist\TanksClone-<version>-win32.zip (make_release.ps1 -Build)
  Pack      packs what dist\Release already holds, without building
  Clean     deletes dist\ and the intermediate build folders (proj.win32\*.win32, cocos2d build output); the next build is a full one

Usage: tools\dist.ps1 -Action Info
       tools\dist.ps1 -Action Release -Version 0.3.0
#>
param(
	[Parameter(Mandatory = $true)][ValidateSet('Info', 'Release', 'Pack', 'Clean')][string]$Action,
	[string]$Version = (Get-Date -Format 'yyyy.MM.dd')
)

. "$PSScriptRoot\common.ps1"
$dist = Join-Path $script:Root 'dist'

switch ($Action) {
	'Info' {
		foreach ($config in 'Debug', 'Test', 'Release') {
			$folder = Join-Path $dist $config
			$exe = Join-Path $folder 'TanksClone.exe'
			if (Test-Path $exe) {
				$size = (Get-ChildItem $folder -Recurse -File | Measure-Object Length -Sum).Sum
				Write-Host ('{0,-8} {1,8:N1} MB  built {2}' -f $config, ($size / 1MB), (Get-Item $exe).LastWriteTime)
			}
			else { Write-Host ('{0,-8} not built' -f $config) }
		}
		Get-ChildItem $dist -Filter *.zip -ErrorAction SilentlyContinue | ForEach-Object { Write-Host ('archive  {0,8:N1} MB  {1}' -f ($_.Length / 1MB), $_.Name) }
	}
	'Release' { Write-Step 'Build Release, then pack the zip'; & (Join-Path $script:Tools 'make_release.ps1') -Version $Version -Build }
	'Pack' { Write-Step 'Packing dist\Release'; & (Join-Path $script:Tools 'make_release.ps1') -Version $Version }
	'Clean' {
		Stop-Game
		$targets = @($dist) + @(Get-ChildItem (Join-Path $script:Root 'proj.win32') -Directory -Filter '*.win32' -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })
		foreach ($target in $targets) {
			if (Test-Path $target) { Write-Step "Deleting $target"; Remove-Item $target -Recurse -Force }
		}
		Write-Ok 'clean (git is not touched; use tools\repo_clean.ps1 for the repository)'
	}
}
