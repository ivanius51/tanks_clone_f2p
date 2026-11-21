<#
Makes the screenshots (and a clip) for a version-history entry: runs dist\Test\TanksClone.exe --showcase and,
with -Clip, turns the captured frames into a video (make_clip.ps1: MP4 with ffmpeg, otherwise a GIF).
The run plays every screen and level and takes a few minutes; do not touch the window meanwhile.

Usage:
  tools\showcase.ps1 -Name loading-screen                 # docs\history\<today>_<NN>_loading-screen\
  tools\showcase.ps1 -Name loading-screen -Clip           # + clip.mp4 (the frames are deleted afterwards)
  tools\showcase.ps1 -Folder C:\somewhere\shots
After it: write notes.txt in the folder and add a line to docs\history\CHANGELOG.md.
#>
param(
	[string]$Name,
	[string]$Folder,
	[switch]$Clip
)

. "$PSScriptRoot\common.ps1"

if (-not (Test-Path $script:TestExe)) { throw 'dist\Test\TanksClone.exe not found: build the Test configuration first' }
if (-not $Folder) {
	if (-not $Name) { throw 'pass -Name <short-name> or -Folder <path>' }
	$history = Join-Path $script:Root 'docs\history'
	$today = Get-Date -Format 'yyyy-MM-dd'
	$next = 1 + @(Get-ChildItem $history -Directory -Filter "${today}_*" -ErrorAction SilentlyContinue).Count
	$Folder = Join-Path $history ('{0}_{1:00}_{2}' -f $today, $next, $Name)
}
New-Item -ItemType Directory -Force $Folder | Out-Null
$Folder = (Resolve-Path $Folder).Path.TrimEnd('\') + '\'

Stop-Game
Write-Step "Showcase into $Folder"
Start-Process $script:TestExe -ArgumentList @('--showcase', "`"$Folder`"") -WorkingDirectory $script:Root -Wait

$frames = Join-Path $Folder 'clip'
if ($Clip -and (Test-Path $frames)) {
	& (Join-Path $script:Tools 'make_clip.ps1') -Frames $frames -Output (Join-Path $Folder 'clip.mp4')
}
# Frames are large and never go into the repository.
if (Test-Path $frames) { Remove-Item $frames -Recurse -Force }
Write-Ok "done: $Folder"
Get-ChildItem $Folder | ForEach-Object { Write-Host "  $($_.Name)" }
