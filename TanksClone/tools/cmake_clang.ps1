param(
    [ValidateSet("configure", "build")]
    [string]$Action = "configure",
    [ValidateSet("Debug", "Test", "Release")]
    [string]$Configuration = "Debug",
    [switch]$Rebuild,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$VsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"

if (!(Test-Path $VsWhere))
{
    throw "Visual Studio Installer was not found. Install the Visual Studio C++ build tools."
}

$VisualStudioRoot = (& $VsWhere -latest -products "*" -property installationPath | Select-Object -First 1)

if (!$VisualStudioRoot)
{
    throw "A Visual Studio installation was not found."
}

$VcVars = Join-Path $VisualStudioRoot "VC\Auxiliary\Build\vcvars32.bat"
$CMake = Join-Path $VisualStudioRoot "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$Clang = "C:\Program Files\LLVM\bin\clang-cl.exe"

if (!(Test-Path $VcVars))
{
    throw "Visual Studio's x86 C++ environment was not found at $VcVars."
}

if (!(Test-Path $CMake))
{
    $CMakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if (!$CMakeCommand)
    {
        throw "CMake was not found. Install CMake or Visual Studio's CMake component."
    }

    $CMake = $CMakeCommand.Source
}

if (!(Test-Path $Clang))
{
    throw "LLVM clang-cl was not found at $Clang. Update CMakePresets.json for its installed path."
}

$Preset = "clang-win32-$($Configuration.ToLowerInvariant())"
$Commands = @(
    "call `"$VcVars`" x86",
    "`"$CMake`" --preset $Preset"
)

if ($Action -eq "build")
{
    $BuildCommand = "`"$CMake`" --build --preset $Preset"
    if ($Rebuild)
    {
        $BuildCommand += " --clean-first"
    }

    $Commands += $BuildCommand
}

if ($DryRun)
{
    Write-Host ($Commands -join " && ")
    return
}

Push-Location $ProjectRoot
try
{
    # vcvars32 writes a harmless vswhere notice to stderr; only the exit code decides.
    $ErrorActionPreference = "Continue"
    & $env:ComSpec /d /c ($Commands -join " && ")
    if ($LASTEXITCODE -ne 0)
    {
        throw "CMake $Action failed with exit code $LASTEXITCODE."
    }
}
finally
{
    Pop-Location
}
