@echo off
rem Starts the interactive tools menu (build, tests, dist, art, repository cleanup). See tools\README.md.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0menu.ps1"
