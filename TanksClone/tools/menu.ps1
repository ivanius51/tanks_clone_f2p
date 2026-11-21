<#
Interactive menu for the project scripts: build, tests, dist, art, repository cleanup, screenshots.
Start it with tools\menu.bat (double-click) or: powershell -ExecutionPolicy Bypass -File tools\menu.ps1
Every item runs one of the scripts of this folder, so the same thing can be done from a script or CI; see tools\README.md.
Type the number (or the letter) and press Enter; q quits.
#>
. "$PSScriptRoot\common.ps1"

function Confirm-Action([string]$Question) {
	return (Read-Host "$Question (y/N)") -match '^(y|yes)$'
}

function Ask([string]$Question, [string]$Default) {
	$answer = Read-Host "$Question [$Default]"
	if ([string]::IsNullOrWhiteSpace($answer)) { return $Default }
	return $answer
}

$tools = $script:Tools

# Each entry: Key, Group, Title, Action. Groups print as headers.
$items = @(
	@{ Key = '1'; Group = 'Build'; Title = 'Build all (Debug, Test, Release)'; Action = { & "$tools\build.ps1" } }
	@{ Key = '2'; Group = 'Build'; Title = 'Build Debug'; Action = { & "$tools\build.ps1" -Configuration Debug } }
	@{ Key = '3'; Group = 'Build'; Title = 'Build Test (what the autotests run on)'; Action = { & "$tools\build.ps1" -Configuration Test } }
	@{ Key = '4'; Group = 'Build'; Title = 'Build Release'; Action = { & "$tools\build.ps1" -Configuration Release } }
	@{ Key = '5'; Group = 'Build'; Title = 'Rebuild everything from scratch'; Action = { if (Confirm-Action 'Full rebuild takes several minutes. Continue?') { & "$tools\build.ps1" -Rebuild } } }

	@{ Key = '6'; Group = 'Tests (Test build)'; Title = 'Unit tests only (fast)'; Action = { & "$tools\test.ps1" -Mode unit } }
	@{ Key = '7'; Group = 'Tests (Test build)'; Title = 'Short: unit + small fields (default --test)'; Action = { & "$tools\test.ps1" -Mode short } }
	@{ Key = '8'; Group = 'Tests (Test build)'; Title = 'Menu and screens'; Action = { & "$tools\test.ps1" -Mode menu } }
	@{ Key = '9'; Group = 'Tests (Test build)'; Title = 'Campaign (the bot plays every level)'; Action = { & "$tools\test.ps1" -Mode campaign } }
	@{ Key = '10'; Group = 'Tests (Test build)'; Title = 'Benchmark (FPS, crowd levels)'; Action = { & "$tools\test.ps1" -Mode benchmark } }
	@{ Key = '11'; Group = 'Tests (Test build)'; Title = 'Crash reporter'; Action = { & "$tools\test.ps1" -Mode crash } }
	@{ Key = '12'; Group = 'Tests (Test build)'; Title = 'Repeat a mode N times (hunt a flaky test)'; Action = {
		$mode = Ask 'Mode (unit, short, menu, campaign)' 'unit'; $count = [int](Ask 'How many runs' '20'); & "$tools\test.ps1" -Mode $mode -Repeat $count } }

	@{ Key = '13'; Group = 'Dist and release'; Title = 'Show dist\ contents'; Action = { & "$tools\dist.ps1" -Action Info } }
	@{ Key = '14'; Group = 'Dist and release'; Title = 'Build Release and pack the zip for players'; Action = { & "$tools\dist.ps1" -Action Release -Version (Ask 'Version' (Get-Date -Format 'yyyy.MM.dd')) } }
	@{ Key = '15'; Group = 'Dist and release'; Title = 'Delete dist\ and intermediate build folders'; Action = { if (Confirm-Action 'Delete dist\ and proj.win32\*.win32?') { & "$tools\dist.ps1" -Action Clean } } }

	@{ Key = '16'; Group = 'Art and sound (Python)'; Title = 'Generate placeholder art (SD)'; Action = { & "$tools\art.ps1" -Action Art } }
	@{ Key = '17'; Group = 'Art and sound (Python)'; Title = 'Generate placeholder art (HD)'; Action = { & "$tools\art.ps1" -Action ArtHd } }
	@{ Key = '18'; Group = 'Art and sound (Python)'; Title = 'Generate placeholder sounds'; Action = { & "$tools\art.ps1" -Action Sounds } }
	@{ Key = '19'; Group = 'Art and sound (Python)'; Title = 'Pack atlases (SD + HD)'; Action = { & "$tools\art.ps1" -Action Pack; & "$tools\art.ps1" -Action PackHd } }
	@{ Key = '20'; Group = 'Art and sound (Python)'; Title = 'Write application icons'; Action = { & "$tools\art.ps1" -Action Icons } }
	@{ Key = '21'; Group = 'Art and sound (Python)'; Title = 'Check atlases and seamless tiles'; Action = { & "$tools\art.ps1" -Action Check } }
	@{ Key = '22'; Group = 'Art and sound (Python)'; Title = 'Everything above, in order'; Action = { if (Confirm-Action 'Regenerates all art, sounds and atlases. Continue?') { & "$tools\art.ps1" -Action All } } }

	@{ Key = '23'; Group = 'Repository (git)'; Title = 'Largest files in the whole history'; Action = { & "$tools\repo_clean.ps1" -Action Report -SizeMB ([double](Ask 'Minimum size in MB' '1')) } }
	@{ Key = '24'; Group = 'Repository (git)'; Title = 'Build output tracked right now'; Action = { & "$tools\repo_clean.ps1" -Action Tracked } }
	@{ Key = '25'; Group = 'Repository (git)'; Title = 'Stop tracking build output (git rm --cached)'; Action = { if (Confirm-Action 'Untrack build output?') { & "$tools\repo_clean.ps1" -Action Untrack } } }
	@{ Key = '26'; Group = 'Repository (git)'; Title = 'Remove build output from the history (dry run)'; Action = { & "$tools\repo_clean.ps1" -Action Rewrite } }
	@{ Key = '27'; Group = 'Repository (git)'; Title = 'Remove build output from the history (REWRITES history)'; Action = {
		if (Confirm-Action 'This rewrites every commit and needs a force-push afterwards. A backup bundle is written first. Continue?') { & "$tools\repo_clean.ps1" -Action Rewrite -Execute } } }

	@{ Key = '28'; Group = 'Version history'; Title = 'Screenshots (+ clip) for a history entry'; Action = {
		$name = Ask 'Short name of the iteration' 'iteration'; if (Confirm-Action 'Also make a clip?') { & "$tools\showcase.ps1" -Name $name -Clip } else { & "$tools\showcase.ps1" -Name $name } } }
	@{ Key = '29'; Group = 'Version history'; Title = 'Open tools\README.md'; Action = { Invoke-Item "$tools\README.md" } }
)

while ($true) {
	Clear-Host
	Write-Host '=== TanksClone tools ===' -ForegroundColor Cyan
	Write-Host "project: $($script:Root)`n"
	$group = ''
	foreach ($item in $items) {
		if ($item.Group -ne $group) { $group = $item.Group; Write-Host "`n$group" -ForegroundColor Yellow }
		Write-Host ('  {0,3}  {1}' -f $item.Key, $item.Title)
	}
	Write-Host "`n    q  Quit"

	$choice = (Read-Host "`nChoice").Trim().ToLower()
	if ($choice -in 'q', 'quit', 'exit') { break }
	$selected = $items | Where-Object { $_.Key -eq $choice } | Select-Object -First 1
	if (-not $selected) { continue }

	Write-Host ''
	try { & $selected.Action }
	catch { Write-Bad "ERROR: $($_.Exception.Message)" }
	Read-Host "`nPress Enter to return to the menu" | Out-Null
}
