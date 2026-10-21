# Capture the README screenshot

[Docs home](../README.md)

The README screenshot is captured from the Terlox window running
[`examples/showcase.py`](../../examples/showcase.py). The example displays the
OS, installed shell paths and project folders.

The background is a separate desktop window, drawn by
[`branding/presentation/Backdrop.cpp`](../../branding/presentation/Backdrop.cpp).
The desktop compositor applies Terlox's transparency and blur over that window.

The capture tool links the same `Window`, `TerminalView`, `Session` and renderer
used by the application. It starts the Python example as a child process,
waits for its prompt and captures the screen area covering the two windows.

## Reproduce it on Windows

Enable the optional developer tool in a compiler-enabled terminal:

```sh
cmake --preset release -DTERLOX_BUILD_PRESENTATION=ON -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build --preset release --target terlox_presentation
```

With Qt's `bin` directory on PATH, run:

```powershell
$project = (Get-Location).Path
$python = (Get-Command python).Source
& ./build-release/bin/terlox_presentation.exe --project $project --python $python --output docs/assets/terlox-windows.png
```

Run from the project root with the desktop unlocked and the capture area
unobstructed. The tool opens two temporary windows, saves
the screenshot and exits. It reads your normal Terlox appearance preferences;
it does not change them or install a desktop wallpaper.

The README capture uses Cascadia Mono 12 pt, a 0.45 black tint and 0.85 native
blur intensity. A different compositor, scaling setting or saved appearance
will produce a different image. See [Platform support](../reference/platforms.md)
for the blur backend on each OS.
