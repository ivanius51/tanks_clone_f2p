#!/bin/sh
# Starts the interactive tools menu from Git Bash / WSL on Windows. See tools/README.md.
# Usage: tools/menu.sh            the menu
#        tools/menu.sh test.ps1 -Mode unit      run one script directly
here="$(cd "$(dirname "$0")" && pwd)"
if [ "$#" -eq 0 ]; then
	exec powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$here/menu.ps1" 2>/dev/null || wslpath -w "$here/menu.ps1")"
fi
script="$1"
shift
exec powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$here/$script" 2>/dev/null || wslpath -w "$here/$script")" "$@"
