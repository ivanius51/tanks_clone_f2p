<#
Turns the clip frames written by "TanksClone.exe --showcase <folder>" into a short video.

  With ffmpeg in PATH: MP4 (H.264, yuv420p, 30 FPS), the preferred format.
  Without it:          an animated GIF from every 4th frame (tools/make_gif.py, standard Python only).

Usage: tools\make_clip.ps1 -Frames <folder>\clip -Output docs\history\<iteration>\clip.mp4
       (with the GIF fallback the extension of -Output is changed to .gif)
#>
param(
	[Parameter(Mandatory = $true)][string]$Frames,
	[Parameter(Mandatory = $true)][string]$Output,
	[int]$Fps = 30
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$ffmpeg = Get-Command ffmpeg -ErrorAction SilentlyContinue

if ($ffmpeg) {
	$mp4 = [IO.Path]::ChangeExtension($Output, '.mp4')
	# Even width and height are required by yuv420p.
	& $ffmpeg.Source -y -loglevel error -framerate $Fps -i (Join-Path $Frames 'frame_%04d.png') `
		-vf 'scale=trunc(iw/2)*2:trunc(ih/2)*2' -c:v libx264 -pix_fmt yuv420p -movflags +faststart $mp4
	if ($LASTEXITCODE -ne 0) { throw "ffmpeg failed with code $LASTEXITCODE" }
	Write-Host "video: $mp4"
}
else {
	$gif = [IO.Path]::ChangeExtension($Output, '.gif')
	python (Join-Path $root 'tools\make_gif.py') $Frames $gif --fps ($Fps / 4) --every 4 --scale 0.5
	if ($LASTEXITCODE -ne 0) { throw "make_gif.py failed" }
	Write-Host "ffmpeg not found, wrote a GIF instead: $gif"
}
