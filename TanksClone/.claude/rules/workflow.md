# Workflow

Before considering a task done:

0. **Project properties**
  - %ProjectName% - at now is "TanksClone"

1. **Self review** 
  -against the checklist: naming (per code-style.md), comments (§19), no commented-out code or macros, encapsulation, no new singletons, layers (logic doesn't depend on the engine), a `/** */` doc comment for every new main class.

2. **Build Configurations** 
   - `Test` must compile: stop `%ProjectName%.exe`, then 
   `MSBuild proj.win32\%ProjectName%.sln /p:Configuration=<Configuration> /p:Platform=Win32 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0.22621.0 /m`
   - Configurations: `Debug`, `Test` (Release optimizations + `--test` and F3), `Release` (no debugging tools). After the build, a ready copy is in `dist\<Configuration>\`.
   - All this can be done by using %ProjectName%\tools 
   - Build all three configurations on purpose: Debug, Test, Release. 
    - Use Debug for crash hunting only when the Test build can't reproduce the crash.
    - Release are only checked for building; There no tests at all.
   
3. **Tests**
- Run autotests only on the Test build: `dist\Test\%ProjectName%.exe --test[=mode] --background`.
- Always add `--background` when no screenshot is needed (the window goes behind other apps and does not take focus).
- After an iteration, run only the test mode that fits that iteration:
  - `--test` (short, default): unit tests + small field cases. Normal code changes.
  - `--test=unit`: only the unit tests (fast; for repeating a flaky unit test many times).
  - `--test=menu`: menu/screen changes.
  - `--test=editor`: level editor changes (layout of docks and tiles, collapse handles, Library, opening every level); a few seconds, no menu walk.
  - `--test=benchmark`: performance and FPS work (crowd levels).
  - `--test=campaign`: levels, bot, progression through the campaign.
  - `--test=audio`: sound changes. Plays real sounds on every output device and every file format (sounds/test: wav 8/16 bit, ogg, mp3), at once and after a background preload; broken and missing files.
  - `--test=crash`: crash reporter changes. Child processes crash on purpose (`--crash-probe=access|exception|terminate`); each must exit with code 2 and leave a report naming the probe function plus a minidump.
- Every test run has a wall-clock cap: the run is killed with exit code 124 and a TIMEOUT line in `test_report.txt` after 30s; `--max-time=<seconds>` changes it. Run long tests in the background, never wait on them in the foreground.
- If no mode fits the change, add a new split `--test=<name>` mode for it instead of running full.
- `--test=full` only in a corner case, after asking the user, preferably with `--background`. Never run it by default before finishing an iteration.
- Put as much logic as possible into fast unit tests.

4. **Version history:** for every BIG new feature create `docs/history/<YYYY-MM-DD>_<NN>_<short-name>/` with 
  - 1-3 screenshots (png)
  - a clip (mp4) if possible and need show animations
  - note in `notes.txt` (a few lines in English: what was done)
  - add a line to `docs/history/CHANGELOG.md`. 
  - To create Screenshots and clip: `dist\Test\%ProjectName%.exe --showcase <folder>` (clip frames go to `<folder>/clip/`, 30 per second of game time); video: `tools\make_clip.ps1 -Frames <folder>\clip -Output <iteration>\clip.mp4` (MP4 via ffmpeg; without ffmpeg it writes a GIF).
4b. **Strict check:** 
  - `python tools/clang_check.py` (clang-cl, syntax + extra warnings, no linking) must report 0 problems.
  - Wait until all build, test processes is finished

5. **Report:** state honestly what was checked, what wasn't, and which tests fail.

Add new files to `.vcxproj` and `.vcxproj.filters` (folders in Visual Studio mirror the structure on disk).
