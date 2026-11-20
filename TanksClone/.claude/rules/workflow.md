# Workflow

Before considering a task done:

1. **Self review** against the checklist: naming (per code-style.md), comments (§19), no commented-out code or macros, encapsulation, no new singletons, layers (logic doesn't depend on the engine), a `/** */` doc comment for every new main class.
2. **Build all three configurations** (`Debug`, `Test`, `Release`; each must compile): stop `TanksClone.exe`, then for each
   `MSBuild proj.win32\TanksClone.sln /p:Configuration=<Configuration> /p:Platform=Win32 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0.22621.0 /m`
   Configurations: `Debug`, `Test` (Release optimizations + `--test` and F3), `Release` (no debugging tools). After the build, a ready copy is in `dist\<Configuration>\`.
3. **Tests:** run the autotests only in the Test build: `dist\Test\TanksClone.exe --test --background` (full run) or `--test=short --background` (unit, small test fields and menu tests; skips the campaign levels and the crowd test). `--background` keeps the window behind others and returns the focus; don't use it with `--showcase`. Report in `test_report.txt`, exit code 0 = everything passed). `--test` doesn't work in Release. New logic comes with new tests; a bug is first reproduced by a test.
4. **Version history:** for every finished iteration create `docs/history/<YYYY-MM-DD>_<NN>_<short-name>/` with 1-3 screenshots (png), a clip (gif) if possible, and `notes.txt` (a few lines in English: what was done), and add a line to `docs/history/CHANGELOG.md`. Screenshots and clip: `dist\Test\TanksClone.exe --showcase <folder>` (clip frames go to `<folder>/clip/`, 30 per second of game time); video: `tools\make_clip.ps1 -Frames <folder>\clip -Output <iteration>\clip.mp4` (MP4 via ffmpeg; without ffmpeg it writes a GIF). Don't put clip frames in the repository.
5. **Report:** state honestly what was checked, what wasn't, and which tests fail.

Add new files to `.vcxproj` and `.vcxproj.filters` (folders in Visual Studio mirror the structure on disk).
