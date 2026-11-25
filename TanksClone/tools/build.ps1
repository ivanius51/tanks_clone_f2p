<#
Builds the game project with MSBuild (the engine comes prebuilt from libs\, see engine_libs.ps1) and leaves a ready copy in dist\<Configuration>\ (the post-build step runs make_dist.ps1).

  Debug    debug build, tests with symbols
  Test     Release optimizations + the --test autotests and F3 tools (what the autotests run on)
  Release  the player's build, no debug tools
  All      Debug, Test and Release in this order (what the workflow asks before an iteration is done)
  CMakeDebug / CMakeTest / CMakeRelease / CMakeAll
           CMake builds using LLVM clang-cl and the x86 Windows SDK

While it builds you see a progress bar (elapsed time, the file being compiled, a percentage once an earlier build has
told how many files there are). The full MSBuild output goes to %TEMP%\tanks_logs\build_<Configuration>.log; on a
failure the errors are printed.

Usage:
  tools\build.ps1                         # all three configurations
  tools\build.ps1 -Configuration Test
  tools\build.ps1 -Configuration Release -Rebuild    # clean build
  tools\build.ps1 -Configuration CMakeDebug
  tools\build.ps1 -Configuration CMakeTest
  tools\build.ps1 -Configuration CMakeRelease -Rebuild
  tools\build.ps1 -DryRun                 # only print the MSBuild commands
#>
param(
	[ValidateSet('Debug', 'Test', 'Release', 'All', 'CMakeDebug', 'CMakeTest', 'CMakeRelease', 'CMakeAll')][string]$Configuration = 'All',
	[switch]$Rebuild,
	[switch]$DryRun
)

. "$PSScriptRoot\common.ps1"

if ($Configuration -like 'CMake*') {
	$cmakeConfigurations = switch ($Configuration) {
		'CMakeDebug' { @('Debug') }
		'CMakeTest' { @('Test') }
		'CMakeRelease' { @('Release') }
		'CMakeAll' { @('Debug', 'Test', 'Release') }
	}

	foreach ($config in $cmakeConfigurations) {
		$arguments = @{ Action = 'build'; Configuration = $config }
		if ($Rebuild) { $arguments.Rebuild = $true }
		if ($DryRun) { $arguments.DryRun = $true }
		Write-Step "CMake/Clang $config"
		& "$PSScriptRoot\cmake_clang.ps1" @arguments
	}
	return
}

$configurations = if ($Configuration -eq 'All') { @('Debug', 'Test', 'Release') } else { @($Configuration) }
$msbuild = Find-MSBuild
Write-Step "MSBuild: $msbuild"
$summary = @()
$failed = @()
$total = [Diagnostics.Stopwatch]::StartNew()

# Compiler echoes one file name per line; a project finishing prints "<name>.vcxproj -> <output>".
$onLine = {
	param($Line, $State)
	if ($Line -match '^\s*([\w\-\. ]+\.(cpp|cc|cxx|c))\s*$') { $State.Count++; $State.Current = 'compiling ' + $Matches[1] }
	elseif ($Line -match '^\s*([\w\-\.]+)\.vcxproj -> ') { $State.Current = 'built ' + $Matches[1] }
	elseif ($Line -match '^\s*(Link|Lib)\b' -or $Line -match 'Generating code|Linking') { $State.Current = 'linking' }
}

$project = Join-Path $script:Root 'proj.win32\TanksClone.vcxproj'
foreach ($config in $configurations) {
	$engineConfig = if ($config -eq 'Debug') { 'Debug' } else { 'Release' }
	if (-not $DryRun) { & "$PSScriptRoot\engine_libs.ps1" -Configuration $engineConfig }
	$target = if ($Rebuild) { 'Rebuild' } else { 'Build' }
	$arguments = @($project, "/t:$target", "/p:Configuration=$config", '/p:Platform=Win32',
		'/p:PlatformToolset=v143', '/p:WindowsTargetPlatformVersion=10.0.22621.0', '/m', '/nologo', '/v:n')
	Write-Step "$target $config"
	if ($DryRun) { Write-Host "$msbuild $($arguments -join ' ')"; continue }

	Stop-Game
	$result = Invoke-Logged -Title "Building $config" -File $msbuild -Arguments $arguments -LogName "build_$config" -OnLine $onLine -CountKey "build_${config}_$target"
	$warnings = @(Select-String -Path $result.Log -Pattern ': warning ' -ErrorAction SilentlyContinue).Count

	if ($result.ExitCode -eq 0) {
		Write-Ok ("{0} OK in {1}: {2} file(s) compiled, {3} warning(s) -> dist\{0}" -f $config, (Format-Duration $result.Duration), $result.State.Count, $warnings)
		$summary += ('{0,-8} OK     {1}' -f $config, (Format-Duration $result.Duration))
	}
	else {
		Write-Bad ("{0} FAILED (exit code {1}) after {2}" -f $config, $result.ExitCode, (Format-Duration $result.Duration))
		Select-String -Path $result.Log -Pattern ': (fatal )?error ' | Select-Object -First 15 | ForEach-Object { Write-Host ('  ' + $_.Line.Trim()) -ForegroundColor Yellow }
		Write-Host "  full log: $($result.Log)"
		$failed += $config
		$summary += ('{0,-8} FAILED {1}' -f $config, (Format-Duration $result.Duration))
	}
}

if ($configurations.Count -gt 1 -and -not $DryRun) {
	Write-Step ("Summary, total {0}" -f (Format-Duration $total.Elapsed))
	$summary | ForEach-Object { Write-Host "  $_" }
}
if ($failed.Count -gt 0) { throw "build failed: $($failed -join ', ')" }
