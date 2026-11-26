<#
Runs the autotests of the Test build (dist\Test\TanksClone.exe --test[=mode] --background) and prints a short result.
While it runs you see a progress bar with the elapsed time, the number of tests finished and the test running now
(a percentage once an earlier run of the same mode has told how many tests there are). The game output goes to a
log file, which this script summarizes.

Modes:
  unit       only the unit tests (fast)
  short      unit tests + the small test fields (the default "--test")
  menu       menu and screen tests
  campaign   the bot plays every built-in level
  benchmark  FPS on the crowd levels
  crash      the crash reporter (child processes crash on purpose)
  audio      sound on every output device and file format (wav, ogg, mp3), sync and async loading
  full       everything above except crash; slow, ask before using it

Exit code: 0 all passed, 1 some test failed, 2 the game crashed.

Usage:
  tools\test.ps1                       # short
  tools\test.ps1 -Mode unit -Repeat 20 # repeat to hunt a flaky test
  tools\test.ps1 -Mode menu -Foreground
Logs: %TEMP%\tanks_logs\test_<mode>_<n>.log
#>
param(
	[ValidateSet('unit', 'short', 'menu', 'campaign', 'benchmark', 'simbench', 'crash', 'audio', 'full')][string]$Mode = 'short',
	[int]$Repeat = 1,
	[switch]$Foreground
)

. "$PSScriptRoot\common.ps1"

if (-not (Test-Path $script:TestExe)) { throw "No Test build in dist\Test or dist\Clang_Test: run tools\build.ps1 -Configuration CMakeTest first" }
Write-Host "Using $script:TestExe"

$argument = if ($Mode -eq 'short') { '--test' } else { "--test=$Mode" }
$arguments = @($argument)
if (-not $Foreground) { $arguments += '--background' }

# "[PASS] name" / "[FAIL] name" lines; the details of a failure follow on the lines after it.
$onLine = {
	param($Line, $State)
	if ($Line -match '^\s*\[(PASS|FAIL)\]\s*(.*)$') {
		$State.Count++
		$State.Current = $Matches[2]
	}
	elseif ($Line -match '^====\s*(.+?)\s*====') { $State.Current = $Matches[1] }
	elseif ($Line -match 'CRASH') { $State.Current = 'CRASH' }
}

$passed = 0; $failed = 0; $crashed = 0
$total = [Diagnostics.Stopwatch]::StartNew()
Write-Step "Test mode '$Mode' x $Repeat"
for ($run = 1; $run -le $Repeat; $run++) {
	Stop-Game
	$title = if ($Repeat -gt 1) { "Tests: $Mode, run $run of $Repeat" } else { "Tests: $Mode" }
	$result = Invoke-Logged -Title $title -File $script:TestExe -Arguments $arguments -LogName "test_${Mode}_$run" -OnLine $onLine -CountKey "test_$Mode"
	$summary = (Select-String -Path $result.Log -Pattern 'tests.*:|TOTAL' -ErrorAction SilentlyContinue | ForEach-Object { $_.Line.Trim() }) -join ' | '
	$time = Format-Duration $result.Duration

	if ($result.ExitCode -eq 0) { $passed++; Write-Ok "run ${run}: PASS  ($($result.State.Count) tests, $time)  $summary" }
	elseif ($result.ExitCode -eq 2) {
		$crashed++
		Write-Bad "run ${run}: CRASH (exit 2) after $($result.State.Count) tests, $time"
		Select-String -Path $result.Log -Pattern 'CRASH' -Context 0, 4 | Select-Object -First 1 | ForEach-Object { $_.Context.PostContext | ForEach-Object { Write-Host "    $($_.Trim())" -ForegroundColor Yellow } }
		Write-Host "    log: $($result.Log)"
	}
	else {
		$failed++
		Write-Bad "run ${run}: FAIL  ($($result.State.Count) tests, $time)  $summary"
		Select-String -Path $result.Log -Pattern '^\s*\[FAIL\]' -Context 0, 2 | ForEach-Object {
			Write-Host "    $($_.Line.Trim())" -ForegroundColor Yellow
			$_.Context.PostContext | ForEach-Object { Write-Host "      $($_.Trim())" -ForegroundColor DarkYellow }
		}
		Write-Host "    log: $($result.Log)"
	}
}

if ($Repeat -gt 1) { Write-Step ("{0} x {1} in {2}: passed {3}, failed {4}, crashed {5}" -f $Mode, $Repeat, (Format-Duration $total.Elapsed), $passed, $failed, $crashed) }
if ($crashed -gt 0) { exit 2 }
if ($failed -gt 0) { exit 1 }
exit 0
