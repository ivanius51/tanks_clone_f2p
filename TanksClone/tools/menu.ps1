<#
Interactive menu for the project scripts. Two levels: pick a category, then the action.
Start it with tools\menu.bat (double-click) or: powershell -ExecutionPolicy Bypass -File tools\menu.ps1
Every action runs one of the scripts of this folder, so the same thing can be done from a script or CI; see tools\README.md.

Keys: a number picks an entry, b goes back, q quits.
#>
. "$PSScriptRoot\common.ps1"

$tools = $script:Tools
$width = 78

# ---------------------------------------------------------------- small helpers

function Confirm-Action([string]$Question) {
	Write-Host $Question -ForegroundColor Yellow -NoNewline
	return (Read-Host ' (y/N)') -match '^(y|yes)$'
}

function Ask([string]$Question, [string]$Default) {
	$answer = Read-Host "$Question [$Default]"
	if ([string]::IsNullOrWhiteSpace($answer)) { return $Default }
	return $answer
}

function Write-Line([string]$Char = '-') { Write-Host ($Char * $width) -ForegroundColor DarkGray }

function Write-Title([string]$Text, [string]$Subtitle) {
	Clear-Host
	Write-Line '='
	Write-Host "  $Text" -ForegroundColor Cyan
	if ($Subtitle) { Write-Host "  $Subtitle" -ForegroundColor DarkGray }
	Write-Line '='
}

# One line of status for the header: branch, and how old the Test build is.
function Get-Status {
	$branch = (git -C $script:Root rev-parse --abbrev-ref HEAD 2>$null)
	$changes = @(git -C $script:Root status --porcelain 2>$null).Count
	$exe = $script:TestExe
	$build = if (Test-Path $exe) { 'Test build ' + [int]((Get-Date) - (Get-Item $exe).LastWriteTime).TotalMinutes + ' min old' } else { 'no Test build yet' }
	return "branch $branch, $changes changed file(s), $build"
}

# ---------------------------------------------------------------- the menu content

# Item: Key, Title, Desc (one line: what it does and how long it takes), Action.
$categories = @(
	@{
		Key = '1'; Name = 'Build'; Desc = 'compile the game (Debug, Test, Release)'
		Items = @(
			@{ Key = '1'; Title = 'Build all three'; Desc = 'Debug, Test and Release; the check before an iteration is done. A few minutes.'; Action = { & "$tools\build.ps1" } }
			@{ Key = '2'; Title = 'Build Test'; Desc = 'the build the autotests run on (Release speed + test tools). The usual choice.'; Action = { & "$tools\build.ps1" -Configuration Test } }
			@{ Key = '3'; Title = 'Build Debug'; Desc = 'slow build with full symbols; for hunting crashes the Test build cannot reproduce.'; Action = { & "$tools\build.ps1" -Configuration Debug } }
			@{ Key = '4'; Title = 'Build Release'; Desc = 'the players build: no debug tools, no tests.'; Action = { & "$tools\build.ps1" -Configuration Release } }
			@{ Key = '5'; Title = 'Rebuild everything'; Desc = 'clean build of all three from scratch. Slow.'; Action = { if (Confirm-Action 'A full rebuild takes several minutes. Continue?') { & "$tools\build.ps1" -Rebuild } } }
		)
	}
	@{
		Key = '2'; Name = 'Tests'; Desc = 'run the autotests of the Test build'
		Items = @(
			@{ Key = '1'; Title = 'Unit tests'; Desc = 'only the unit tests. Seconds. Use after most code changes.'; Action = { & "$tools\test.ps1" -Mode unit } }
			@{ Key = '2'; Title = 'Short'; Desc = 'unit tests + the small test fields (the default --test). About a minute.'; Action = { & "$tools\test.ps1" -Mode short } }
			@{ Key = '3'; Title = 'Menu and screens'; Desc = 'clicks through the menus, settings, upgrades and a level.'; Action = { & "$tools\test.ps1" -Mode menu } }
			@{ Key = '4'; Title = 'Campaign'; Desc = 'the bot plays every built-in level. Slow; for level and progression changes.'; Action = { & "$tools\test.ps1" -Mode campaign } }
			@{ Key = '5'; Title = 'Benchmark'; Desc = 'FPS on the crowd levels. Needs the window in focus for real numbers.'; Action = { & "$tools\test.ps1" -Mode benchmark } }
			@{ Key = '6'; Title = 'Crash reporter'; Desc = 'child processes crash on purpose; checks the crash report and dump.'; Action = { & "$tools\test.ps1" -Mode crash } }
			@{ Key = '7'; Title = 'Audio'; Desc = 'plays sounds on every output device and format (wav, ogg, mp3), sync and async loading.'; Action = { & "$tools\test.ps1" -Mode audio } }
			@{ Key = '8'; Title = 'Strict compile check'; Desc = 'clang-cl syntax and extra warnings over Classes\ (no linking); must report 0 problems.'; Action = { Invoke-ToolPython 'clang_check.py' } }
			@{ Key = '9'; Title = 'Repeat a mode N times'; Desc = 'run one mode again and again to catch a flaky test.'; Action = {
				$mode = Ask 'Mode (unit, short, menu, campaign, audio)' 'unit'; $count = [int](Ask 'How many runs' '20'); & "$tools\test.ps1" -Mode $mode -Repeat $count } }
		)
	}
	@{
		Key = '3'; Name = 'Dist and release'; Desc = 'what is in dist\, the zip for players, cleaning build folders'
		Items = @(
			@{ Key = '1'; Title = 'Show dist contents'; Desc = 'size and build time of dist\Debug, Test and Release, and any zip archives.'; Action = { & "$tools\dist.ps1" -Action Info } }
			@{ Key = '2'; Title = 'Build Release and pack the zip'; Desc = 'builds Release, then dist\TanksClone-<version>-win32.zip for players.'; Action = { & "$tools\dist.ps1" -Action Release -Version (Ask 'Version' (Get-Date -Format 'yyyy.MM.dd')) } }
			@{ Key = '3'; Title = 'Pack the zip without building'; Desc = 'zips what dist\Release already holds.'; Action = { & "$tools\dist.ps1" -Action Pack -Version (Ask 'Version' (Get-Date -Format 'yyyy.MM.dd')) } }
			@{ Key = '4'; Title = 'Delete build folders'; Desc = 'removes dist\ and proj.win32\*.win32; the next build is a full one. Git is not touched.'; Action = { if (Confirm-Action 'Delete dist\ and proj.win32\*.win32?') { & "$tools\dist.ps1" -Action Clean } } }
		)
	}
	@{
		Key = '4'; Name = 'Art and sound'; Desc = 'placeholder art, sounds, icons, atlases (Python)'
		Items = @(
			@{ Key = '1'; Title = 'Generate art (SD)'; Desc = 'draws the placeholder pixel art into art\.'; Action = { & "$tools\art.ps1" -Action Art } }
			@{ Key = '2'; Title = 'Generate art (HD)'; Desc = 'draws the HD placeholder art into art_hd\.'; Action = { & "$tools\art.ps1" -Action ArtHd } }
			@{ Key = '3'; Title = 'Generate sounds'; Desc = 'synthesizes the placeholder sound effects into Resources\sounds\.'; Action = { & "$tools\art.ps1" -Action Sounds } }
			@{ Key = 'm'; Title = 'Generate music'; Desc = 'synthesizes the calm and intense music loops into Resources\music\ (OGG with ffmpeg).'; Action = { & "$tools\art.ps1" -Action Music } }
			@{ Key = 'u'; Title = 'Generate upgrade art'; Desc = 'draws the upgrade screen blueprints into art\ and art_hd\; pack the atlases after.'; Action = { & "$tools\art.ps1" -Action Upgrades } }
			@{ Key = '4'; Title = 'Pack atlases'; Desc = 'packs art\ and art_hd\ into the sprite atlases the game loads.'; Action = { & "$tools\art.ps1" -Action Pack; & "$tools\art.ps1" -Action PackHd } }
			@{ Key = '5'; Title = 'Write app icons'; Desc = 'writes the application icon into every platform project (generate the art first).'; Action = { & "$tools\art.ps1" -Action Icons } }
			@{ Key = '6'; Title = 'Check atlases and tiles'; Desc = 'verifies the atlases are up to date and the ground tiles are seamless. Read-only.'; Action = { & "$tools\art.ps1" -Action Check } }
			@{ Key = '7'; Title = 'Do everything'; Desc = 'art, HD art, upgrade art, sounds, music, atlases, icons, checks, in order. Overwrites generated files.'; Action = { if (Confirm-Action 'Regenerates all art, sounds, music and atlases. Continue?') { & "$tools\art.ps1" -Action All } } }
		)
	}
	@{
		Key = '5'; Name = 'Repository'; Desc = 'find and remove binaries from git (big files, build output)'
		Items = @(
			@{ Key = '1'; Title = 'Largest files in the history'; Desc = 'which files make the repository big, all versions added up. Read-only.'; Action = { & "$tools\repo_clean.ps1" -Action Report -SizeMB ([double](Ask 'Minimum size in MB' '1')) } }
			@{ Key = '2'; Title = 'Build output tracked now'; Desc = 'exe, dll, pdb, obj and similar files that are tracked at the moment. Read-only.'; Action = { & "$tools\repo_clean.ps1" -Action Tracked } }
			@{ Key = '3'; Title = 'Stop tracking build output'; Desc = 'git rm --cached for the files above; they stay on disk, nothing is committed.'; Action = { if (Confirm-Action 'Untrack build output?') { & "$tools\repo_clean.ps1" -Action Untrack } } }
			@{ Key = '4'; Title = 'History cleanup: dry run'; Desc = 'lists what a history rewrite would remove and how much it saves. Changes nothing.'; Action = { & "$tools\repo_clean.ps1" -Action Rewrite } }
			@{ Key = '5'; Title = 'History cleanup: REWRITE'; Desc = 'removes build output from every commit (git filter-repo). Backup bundle first; needs a force-push after.'; Action = {
				if (Confirm-Action 'This rewrites every commit and needs a force-push afterwards. Continue?') { & "$tools\repo_clean.ps1" -Action Rewrite -Execute } } }
		)
	}
	@{
		Key = '6'; Name = 'History and docs'; Desc = 'screenshots and clips for docs\history, the tools guide'
		Items = @(
			@{ Key = '1'; Title = 'Screenshots for a history entry'; Desc = 'plays every screen and level in the Test build and saves PNGs. A few minutes; leave the window alone.'; Action = {
				$name = Ask 'Short name of the iteration' 'iteration'; & "$tools\showcase.ps1" -Name $name } }
			@{ Key = '2'; Title = 'Screenshots and a clip'; Desc = 'the same, plus a short video (MP4 with ffmpeg, otherwise a GIF).'; Action = {
				$name = Ask 'Short name of the iteration' 'iteration'; & "$tools\showcase.ps1" -Name $name -Clip } }
			@{ Key = '3'; Title = 'Open the tools guide'; Desc = 'tools\README.md: every script and the typical flows.'; Action = { Invoke-Item "$tools\README.md" } }
		)
	}
)

# ---------------------------------------------------------------- screens

function Show-Category($Category) {
	while ($true) {
		Write-Title $Category.Name $Category.Desc
		Write-Host ''
		foreach ($item in $Category.Items) {
			Write-Host ('  {0}  ' -f $item.Key) -ForegroundColor Yellow -NoNewline
			Write-Host $item.Title -ForegroundColor White
			Write-Host ('     {0}' -f $item.Desc) -ForegroundColor DarkGray
		}
		Write-Host ''
		Write-Line
		Write-Host '  b  back      q  quit' -ForegroundColor DarkGray

		$choice = (Read-Host "`nChoose").Trim().ToLower()
		if ($choice -in 'b', 'back', '') { return $true }
		if ($choice -in 'q', 'quit', 'exit') { return $false }
		$selected = $Category.Items | Where-Object { $_.Key -eq $choice } | Select-Object -First 1
		if (-not $selected) { continue }

		Write-Host ''
		Write-Line
		Write-Host "  $($Category.Name): $($selected.Title)" -ForegroundColor Cyan
		Write-Line
		try { & $selected.Action }
		catch { Write-Bad "ERROR: $($_.Exception.Message)" }
		Write-Host ''
		Read-Host 'Press Enter to continue' | Out-Null
	}
}

function Show-Main {
	while ($true) {
		Write-Title 'TanksClone tools' (Get-Status)
		Write-Host ''
		foreach ($category in $categories) {
			Write-Host ('  {0}  ' -f $category.Key) -ForegroundColor Yellow -NoNewline
			Write-Host ('{0,-18}' -f $category.Name) -ForegroundColor White -NoNewline
			Write-Host ('{0}  ({1})' -f $category.Desc, $category.Items.Count) -ForegroundColor DarkGray
		}
		Write-Host ''
		Write-Line
		Write-Host '  q  quit' -ForegroundColor DarkGray

		$choice = (Read-Host "`nChoose a category").Trim().ToLower()
		if ($choice -in 'q', 'quit', 'exit', 'b') { return }
		$selected = $categories | Where-Object { $_.Key -eq $choice } | Select-Object -First 1
		if ($selected -and -not (Show-Category $selected)) { return }
	}
}

Show-Main
