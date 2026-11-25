@echo off
rem Starts the interactive tools menu (build, tests, dist, art, repository cleanup). See tools\README.md.
if /i "%~1"=="cmake-debug" (
	powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" -Configuration CMakeDebug
	exit /b %errorlevel%
)
if /i "%~1"=="cmake-release" (
	powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" -Configuration CMakeRelease
	exit /b %errorlevel%
)
if /i "%~1"=="cmake-all" (
	powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" -Configuration CMakeAll
	exit /b %errorlevel%
)
if not "%~1"=="" (
	echo Usage: menu.bat [cmake-debug^|cmake-release^|cmake-all]
	exit /b 2
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0menu.ps1"
exit /b %errorlevel%
