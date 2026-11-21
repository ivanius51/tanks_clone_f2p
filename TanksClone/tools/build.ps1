<#
Builds the game with MSBuild and leaves a ready copy in dist\<Configuration>\ (the post-build step runs make_dist.ps1).

  Debug    debug build, tests with symbols
  Test     Release optimizations + the --test autotests and F3 tools (what the autotests run on)
  Release  the player's build, no debug tools
  All      Debug, Test and Release in this order (what the workflow asks before an iteration is done)

Usage:
  tools\build.ps1                         # all three configurations
  tools\build.ps1 -Configuration Test
  tools\build.ps1 -Configuration Release -Rebuild    # clean build
  tools\build.ps1 -DryRun                 # only print the MSBuild commands
#>
param(
	[ValidateSet('Debug', 'Test', 'Release', 'All')][string]$Configuration = 'All',
	[switch]$Rebuild,
	[switch]$DryRun
)

. "$PSScriptRoot\common.ps1"

$configurations = if ($Configuration -eq 'All') { @('Debug', 'Test', 'Release') } else { @($Configuration) }
$msbuild = Find-MSBuild
Write-Step "MSBuild: $msbuild"
$failed = @()

foreach ($config in $configurations) {
	$arguments = @($script:Solution, "/t:$(if ($Rebuild) { 'Rebuild' } else { 'Build' })", "/p:Configuration=$config", '/p:Platform=Win32',
		'/p:PlatformToolset=v143', '/p:WindowsTargetPlatformVersion=10.0.22621.0', '/m', '/nologo', '/v:m')
	Write-Step "Building $config"
	if ($DryRun) { Write-Host "$msbuild $($arguments -join ' ')"; continue }

	Stop-Game
	$watch = [Diagnostics.Stopwatch]::StartNew()
	& $msbuild @arguments
	if ($LASTEXITCODE -eq 0) { Write-Ok "$config OK in $([int]$watch.Elapsed.TotalSeconds) s -> dist\$config" }
	else { Write-Bad "$config FAILED (exit code $LASTEXITCODE)"; $failed += $config }
}

if ($failed.Count -gt 0) { throw "build failed: $($failed -join ', ')" }
