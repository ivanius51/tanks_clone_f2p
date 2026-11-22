@echo off
cd /d "%~dp0..\.."
python tools\tileset\gen_tileset.py %*
pause
