<#
Runs the autotests of the Test build (dist\Test\TanksClone.exe --test[=mode] --background) and prints a short result.
The game window is a GUI program: its output goes to a log file, which this script summarizes.

Modes:
  unit       only the unit tests (fast)
  short      unit tests + the small test fields (the default "--test")
  menu       menu and screen tests
  campaign   the bot plays every built-in level
  benchmark  FPS on the crowd levels
  crash      the crash reporter (child processes crash on purpose)
  full       everything above except crash; slow, ask before using it

Exit code: 0 all passed, 1 some test failed, 2 the game crashed.

Usage:
  tools\test.ps1                       # short
  tools\test.ps1 -Mode unit -Repeat 20 # repeat to hunt a flaky test
  tools\test.ps1 -Mode menu -Foreground
Logs: %TEMP%\tanks_tests\<mode>_<n>.txt
#>
param(
	[ValidateSet('unit', 'short', 'menu', 'campaign', 'benchmark', 'crash', 'full')][string]$Mode = 'short',
	[int]$Repeat = 1,
	[switch]$Foreground
)

. "$PSScriptRoot\common.ps1"

if (-not (Test-Path $script:TestExe)) { throw "dist\Test\TanksClone.exe not found: run tools\build.ps1 -Configuration Test first" }
$logs = Join-Path $env:TEMP 'tanks_tests'
New-Item -ItemType Directory -Force $logs | Out-Null

$argument = if ($Mode -eq 'short') { '--test' } else { "--test=$Mode" }
$arguments = @($argument)
if (-not $Foreground) { $arguments += '--background' }

$passed = 0; $failed = 0; $crashed = 0
for ($run = 1; $run -le $Repeat; $run++) {
	$log = Join-Path $logs "${Mode}_$run.txt"
	$err = Join-Path $logs "${Mode}_$run.err.txt"
	Stop-Game
	$process = Start-Process $script:TestExe -ArgumentList $arguments -WorkingDirectory $script:Root -Wait -PassThru `
		-RedirectStandardOutput $log -RedirectStandardError $err
	$text = Get-Content $log -Raw -ErrorAction SilentlyContinue
	$summary = (Select-String -Path $log -Pattern 'tests.*:|TOTAL' -ErrorAction SilentlyContinue | ForEach-Object { $_.Line.Trim() }) -join ' | '
	$failures = @(Select-String -Path $log -Pattern '^\s*\[FAIL\]' -ErrorAction SilentlyContinue | ForEach-Object { $_.Line.Trim() })

	if ($process.ExitCode -eq 0) { $passed++; Write-Ok "run ${run}: PASS  $summary" }
	elseif ($process.ExitCode -eq 2) { $crashed++; Write-Bad "run ${run}: CRASH (exit 2), see $log" }
	else { $failed++; Write-Bad "run ${run}: FAIL  $summary"; $failures | ForEach-Object { Write-Host "    $_" -ForegroundColor Yellow } }
}

if ($Repeat -gt 1) { Write-Step "$Mode x ${Repeat}: passed $passed, failed $failed, crashed $crashed" }
if ($crashed -gt 0) { exit 2 }
if ($failed -gt 0) { exit 1 }
exit 0
