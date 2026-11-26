<#
Shared helpers for the scripts in tools\ (dot-source it: . "$PSScriptRoot\common.ps1").
#>
$ErrorActionPreference = 'Stop'

$script:Root = Split-Path -Parent $PSScriptRoot        # ...\TanksClone
$script:Tools = $PSScriptRoot
$script:Solution = Join-Path $script:Root 'proj.win32\TanksClone.sln'
# The newest Test build that exists: MSVC (dist\Test) or LLVM clang-cl (dist\Clang_Test).
$script:TestExe = @('dist\Test\TanksClone.exe', 'dist\Clang_Test\TanksClone.exe') |
	ForEach-Object { Join-Path $script:Root $_ } | Where-Object { Test-Path $_ } |
	Sort-Object { (Get-Item $_).LastWriteTime } -Descending | Select-Object -First 1
if (-not $script:TestExe) { $script:TestExe = Join-Path $script:Root 'dist\Test\TanksClone.exe' }

$script:LogDir = Join-Path $env:TEMP 'tanks_logs'

function Write-Step($Text) { Write-Host ('[{0}] >> {1}' -f (Get-Date -Format 'HH:mm:ss'), $Text) -ForegroundColor Cyan }
function Write-Ok($Text) { Write-Host $Text -ForegroundColor Green }
function Write-Bad($Text) { Write-Host $Text -ForegroundColor Red }

# The Python launcher or python, whichever exists.
function Get-Python {
	foreach ($name in 'py', 'python', 'python3') {
		$found = Get-Command $name -ErrorAction SilentlyContinue
		if ($found) {
			if ($name -eq 'py') { return @($found.Source, '-3') }
			return @($found.Source)
		}
	}
	throw 'Python 3 was not found in PATH'
}

# Runs a Python script of tools\ from the project root; extra arguments are passed on.
function Invoke-ToolPython([string]$Script, [string[]]$Arguments = @()) {
	$python = Get-Python
	$exe = $python[0]
	$prefix = @($python | Select-Object -Skip 1)
	Push-Location $script:Root
	try {
		& $exe @prefix (Join-Path $script:Tools $Script) @Arguments
		if ($LASTEXITCODE -ne 0) { throw "$Script failed with code $LASTEXITCODE" }
	}
	finally { Pop-Location }
}

function Find-MSBuild {
	$inPath = Get-Command MSBuild -ErrorAction SilentlyContinue
	if ($inPath) { return $inPath.Source }
	$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
	if (Test-Path $vswhere) {
		$found = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
		if ($found) { return $found }
	}
	throw 'MSBuild was not found: install Visual Studio 2022 (Desktop C++) or open a Developer PowerShell'
}

function Stop-Game {
	Get-Process TanksClone -ErrorAction SilentlyContinue | Stop-Process -Force
}

# The root of the git repository (the folder above TanksClone).
function Get-GitRoot {
	$top = git -C $script:Root rev-parse --show-toplevel 2>$null
	if (-not $top) { throw 'not inside a git repository' }
	return ($top -replace '/', '\')
}

function Format-Duration([TimeSpan]$Span) {
	if ($Span.TotalMinutes -ge 1) { return '{0}m {1:00}s' -f [int][math]::Floor($Span.TotalMinutes), $Span.Seconds }
	return '{0:N1}s' -f $Span.TotalSeconds
}

# How many items (files compiled, tests run) the last run of this kind had; it gives the progress bar its 100%.
function Get-LastCount([string]$Key) {
	$file = Join-Path $script:LogDir "$Key.count"
	if (Test-Path $file) { return [int](Get-Content $file -Raw) }
	return 0
}

function Set-LastCount([string]$Key, [int]$Count) {
	if ($Count -gt 0) { Set-Content (Join-Path $script:LogDir "$Key.count") $Count }
}

function ConvertTo-CommandLine([string[]]$Arguments) {
	($Arguments | ForEach-Object { if ($_ -match '[\s"]') { '"' + (($_ -replace '(\\*)"', '$1$1\"') -replace '(\\+)$', '$1$1') + '"' } else { $_ } }) -join ' '
}

<#
Runs a program with its whole output going to a log file (%TEMP%\tanks_logs\<LogName>.log) and shows live progress:
a progress bar with the elapsed time, the current item and a count. The count becomes a percentage once an earlier run
of the same -LogName key has told how many items there are.

  -OnLine  { param($Line, $State) ... }   called for every output line; may set $State.Current and add to $State.Count
  -OnTick  { param($State) ... }          called about 4 times a second; for programs that print nothing (e.g. count files)
Returns an object with ExitCode, Log, Duration and State.
#>
function Invoke-Logged {
	param(
		[Parameter(Mandatory = $true)][string]$Title,
		[Parameter(Mandatory = $true)][string]$File,
		[string[]]$Arguments = @(),
		[Parameter(Mandatory = $true)][string]$LogName,
		[string]$WorkingDirectory = $script:Root,
		[scriptblock]$OnLine,
		[scriptblock]$OnTick,
		[string]$CountKey = $LogName
	)

	New-Item -ItemType Directory -Force $script:LogDir | Out-Null
	$log = Join-Path $script:LogDir "$LogName.log"
	if (Test-Path $log) { Remove-Item $log -Force }
	$expected = Get-LastCount $CountKey
	$state = @{ Count = 0; Current = '' }

	$command = '"{0}" {1} > "{2}" 2>&1' -f $File, (ConvertTo-CommandLine $Arguments), $log
	$watch = [Diagnostics.Stopwatch]::StartNew()
	$process = Start-Process cmd.exe -ArgumentList "/d /c `"$command`"" -WorkingDirectory $WorkingDirectory -PassThru -WindowStyle Hidden

	$deadline = [DateTime]::Now.AddSeconds(5)
	while (-not (Test-Path $log) -and [DateTime]::Now -lt $deadline) { Start-Sleep -Milliseconds 50 }
	$stream = [IO.File]::Open($log, 'Open', 'Read', 'ReadWrite')
	$reader = New-Object IO.StreamReader($stream)
	try {
		$id = Get-Random -Minimum 1 -Maximum 1000
		while ($true) {
			while ($null -ne ($line = $reader.ReadLine())) {
				if ($OnLine) { & $OnLine $line $state }
			}
			if ($OnTick) { & $OnTick $state }

			$status = 'elapsed ' + (Format-Duration $watch.Elapsed)
			if ($state.Count -gt 0) { $status += ', ' + $state.Count + $(if ($expected -gt 0) { ' of about ' + $expected } else { '' }) + ' done' }
			if ($state.Current) { $status += '  ' + $state.Current }
			$percent = if ($expected -gt 0) { [math]::Min(99, [int](100 * $state.Count / $expected)) } else { -1 }
			Write-Progress -Id $id -Activity $Title -Status $status -PercentComplete $percent

			if ($process.HasExited) {
				# one more read: output written just before the exit
				while ($null -ne ($line = $reader.ReadLine())) { if ($OnLine) { & $OnLine $line $state } }
				break
			}
			Start-Sleep -Milliseconds 250
		}
		Write-Progress -Id $id -Activity $Title -Completed
	}
	finally {
		$reader.Dispose()
		$stream.Dispose()
	}
	$watch.Stop()
	Set-LastCount $CountKey $state.Count
	return [pscustomobject]@{ ExitCode = $process.ExitCode; Log = $log; Duration = $watch.Elapsed; State = $state }
}
