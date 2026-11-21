<#
Shared helpers for the scripts in tools\ (dot-source it: . "$PSScriptRoot\common.ps1").
#>
$ErrorActionPreference = 'Stop'

$script:Root = Split-Path -Parent $PSScriptRoot        # ...\TanksClone
$script:Tools = $PSScriptRoot
$script:Solution = Join-Path $script:Root 'proj.win32\TanksClone.sln'
$script:TestExe = Join-Path $script:Root 'dist\Test\TanksClone.exe'

function Write-Step($Text) { Write-Host ">> $Text" -ForegroundColor Cyan }
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
