# tools

Scripts for building, testing, packing and cleaning the project. Start with the menu:

```
tools\menu.bat          (double-click, or run from a terminal)
tools/menu.sh           (Git Bash / WSL)
```

Every menu item runs one of the scripts below, so each can also be used directly (from a terminal or CI). All scripts are
PowerShell (Windows PowerShell 5.1 is enough) or Python 3 (standard library only). Run them from anywhere: they find the project themselves.

Long steps show a progress bar (elapsed time, the file or test in progress, a percentage once an earlier run told the total) and every
step prints a timestamp and its duration. The full output of builds, tests and showcase runs goes to `%TEMP%\tanks_logs\<name>.log`;
on a failure the script prints the errors and the log path. Note: the game buffers its output while it runs, so during `test.ps1` the bar shows
elapsed time and the test counter fills in when the game flushes its report.

## Everyday scripts

| Script | What it does |
| --- | --- |
| `menu.ps1` / `menu.bat` / `menu.sh` | Interactive menu in two levels: pick a category (Build, Tests, Dist and release, Art and sound, Repository, History and docs), then an action; every action shows a one-line description. |
| `build.ps1` | MSBuild for `Debug`, `Test`, `Release` or `All`; `-Rebuild` for a clean build; `-DryRun` prints the commands. The post-build step leaves a ready copy in `dist\<Configuration>\`. |
| `test.ps1` | Runs the autotests of the Test build: `-Mode unit|short|menu|campaign|benchmark|crash|full`, `-Repeat N` to hunt a flaky test, `-Foreground` to watch. Exit code 0 pass, 1 failed, 2 crashed. Logs go to `%TEMP%\tanks_tests`. |
| `dist.ps1` | `Info` (what is in `dist\`), `Release` (build + zip for players), `Pack` (zip without building), `Clean` (delete `dist\` and intermediate build folders). |
| `showcase.ps1` | Screenshots for a history entry (`-Name <short-name>` creates `docs\history\<date>_<NN>_<name>\`), `-Clip` adds a video. |
| `art.ps1` | Placeholder art, HD art, sounds, icons, atlas packing and checks (wraps the Python tools). |
| `repo_clean.ps1` | Finds and removes binaries from git: `Report` (largest files in the history), `Tracked`, `Untrack`, `Rewrite` (history rewrite with git-filter-repo; dry run unless `-Execute`; writes a backup bundle first; never pushes). |
| `common.ps1` | Shared helpers (paths, MSBuild lookup, Python launcher). Not run directly. |

## Lower-level scripts (used by the ones above)

| Script | What it does |
| --- | --- |
| `make_dist.ps1` | Post-build step: mirrors `Resources\` and assembles `dist\<Configuration>\` (exe, DLLs, VC runtime, symbols for Debug/Test). |
| `make_release.ps1` | Zips `dist\Release` into `dist\TanksClone-<version>-win32.zip`; `-Build` builds Release first. |
| `make_clip.ps1` | Turns the frames from `--showcase` into an MP4 (ffmpeg) or, without ffmpeg, a GIF. |
| `make_gif.py` | The GIF fallback of `make_clip.ps1` (standard library only). |
| `gen_placeholder_art.py` | Draws all placeholder pixel art into `art\` (`--hd`: into `art_hd\`, using `hd_art.py`). |
| `hd_art.py` | The HD sprite drawing code used by `gen_placeholder_art.py --hd`; a module, not run directly. |
| `gen_placeholder_sounds.py` | Synthesizes the placeholder sound effects into `Resources\sounds\`. |
| `gen_app_icons.py` | Writes the application icon into every platform project (run after the art is generated). |
| `pack_atlases.py` | Packs `art\` into sprite atlases in `Resources\textures\` (`--hd`: `art_hd\` into `textures_hd\`; `--check`: only verify they are up to date). |
| `check_tileable.py` | Checks that ground tiles are seamless. |
| `pngio.py` | Minimal PNG reader/writer shared by the Python tools; a module, not run directly. |

## Typical flows

- **After a code change:** `tools\build.ps1 -Configuration Test`, then `tools\test.ps1 -Mode unit` (or the mode that fits the change).
- **Before an iteration is done:** `tools\build.ps1` (all three configurations), the fitting `tools\test.ps1` mode, then `tools\showcase.ps1 -Name <name> -Clip` and the notes in `docs\history`.
- **After changing art:** `tools\art.ps1 -Action Art` (and `ArtHd`), `Pack`, `PackHd`, then `Check`.
- **A flaky test:** `tools\test.ps1 -Mode unit -Repeat 20`.
- **The repository is big:** `tools\repo_clean.ps1 -Action Report`, then `Tracked` / `Untrack` for what is tracked now, and `Rewrite` (dry run first) for the history. After a rewrite every clone has to be cloned again.
