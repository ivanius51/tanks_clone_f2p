<#
Art, sound and icon generation (the Python tools of this folder) in one place.

Actions:
  Art       generate the SD placeholder art into art\        (gen_placeholder_art.py)
  ArtHd     generate the HD placeholder art into art_hd\     (gen_placeholder_art.py --hd, uses hd_art.py)
  Sounds    synthesize the placeholder sounds into Resources\sounds\   (gen_placeholder_sounds.py)
  Icons     write the application icon into every platform project     (gen_app_icons.py; run Art first)
  Pack      pack art\ into the SD atlases in Resources\textures\       (pack_atlases.py)
  PackHd    pack art_hd\ into Resources\textures_hd\                   (pack_atlases.py --hd)
  Check     verify that the atlases are up to date and the ground tiles are seamless (pack_atlases.py --check [--hd], check_tileable.py)
  All       Art, ArtHd, Sounds, Pack, PackHd, Icons, Check

Usage: tools\art.ps1 -Action Art
       tools\art.ps1 -Action All
#>
param(
	[Parameter(Mandatory = $true)][ValidateSet('Art', 'ArtHd', 'Sounds', 'Icons', 'Pack', 'PackHd', 'Check', 'All')][string]$Action
)

. "$PSScriptRoot\common.ps1"

function Run-Action([string]$Name) {
	Write-Step $Name
	switch ($Name) {
		'Art' { Invoke-ToolPython 'gen_placeholder_art.py' }
		'ArtHd' { Invoke-ToolPython 'gen_placeholder_art.py' @('--hd') }
		'Sounds' { Invoke-ToolPython 'gen_placeholder_sounds.py' }
		'Icons' { Invoke-ToolPython 'gen_app_icons.py' }
		'Pack' { Invoke-ToolPython 'pack_atlases.py' }
		'PackHd' { Invoke-ToolPython 'pack_atlases.py' @('--hd') }
		'Check' {
			Invoke-ToolPython 'pack_atlases.py' @('--check')
			if (Test-Path (Join-Path $script:Root 'art_hd')) { Invoke-ToolPython 'pack_atlases.py' @('--hd', '--check') }
			Invoke-ToolPython 'check_tileable.py'
		}
	}
}

$actions = if ($Action -eq 'All') { @('Art', 'ArtHd', 'Sounds', 'Pack', 'PackHd', 'Icons', 'Check') } else { @($Action) }
foreach ($name in $actions) { Run-Action $name }
Write-Ok 'done'
