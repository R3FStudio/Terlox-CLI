# Build from source

[Docs home](../README.md) | [Set up dependencies](dependencies.md)

Start with the dependency guide if this is your first build. It covers the
compiler, Qt and command-line tools for each OS. Run these commands from the
project root.

## Windows

Set `QT_ROOT_DIR` to the desktop kit you installed, then run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/Build-Windows.ps1 -Configuration Release -QtRoot $env:QT_ROOT_DIR
```

The script finds Visual Studio, builds the app and runs the tests. You can use
a regular PowerShell window; the script loads the compiler environment itself.
Add `-SkipTests` when you only need to rebuild. The app is written to
`build-release/bin/terlox.exe`.

The build also needs the Windows console runtime. The script downloads the
repository's selected package and checks its checksum; later builds reuse it.

## Linux and macOS

After setting `QT_ROOT_DIR` and adding its `bin` directory to PATH:

```sh
cmake --preset release -DCMAKE_PREFIX_PATH="$QT_ROOT_DIR"
cmake --build --preset release --parallel
ctest --test-dir build-release --output-on-failure
```

On Linux the app is `build-release/bin/terlox`. On macOS it is
`build-release/bin/terlox.app`. Keep your Qt installation available when running
these development builds.

## Make an installer

```sh
python scripts/Package.py --skip-build --check --qt-root "$QT_ROOT_DIR"
```

On Windows, use `$env:QT_ROOT_DIR` in place of `$QT_ROOT_DIR`. On a Unix system
where Python is named `python3`, use that name for the repository's Python
commands.

Finished installers go in `dist/`. The `build-release/bin` folder also contains
tests and development tools; distribute the packaged files instead. See
[Package and release](releases.md) for all-platform builds and publishing.

## Run the checks you need

| Suite | Covers |
| --- | --- |
| `core` | Shell parsing, screen basics and launch requests |
| `processes` | Real shell input/output, environment and CLI programs |
| `gui` | Rendering, selection, zoom, menus and window behavior |
| `native-shells` | PowerShell/CMD scripts and permission handling |
| `protocols` | Text colors/styles, appearance controls and image messages |
| `installer` | Folder checks, copying, upgrades and removal |
| `setup-wizard` | Wizard pages and installation choices |
| `default-terminal` | Default-terminal preference handling |
| `updates` | Release parsing, platform downloads and bounded HTTP lookup |

For example, after changing the renderer:

```sh
ctest --test-dir build-release --output-on-failure -R "processes|gui|protocols"
```

Run GUI checks on a desktop to check dragging and transparency. For a headless
runner, set `QT_QPA_PLATFORM=offscreen` and `QT_QUICK_BACKEND=software`; those
settings skip checks that need a real desktop.

GitHub Actions uses the same native build and test steps. The
[workflow](../../.github/workflows/build.yml) lists the Qt kit and build
environment used for each platform.

## Test default-terminal integration

On an interactive Windows test desktop, package the build and run:

```sh
python tests/WindowsDefaultTerminal.py --directory build-release/package/windows-x64-VERSION
```

Replace `VERSION` with the package version. This opt-in test temporarily selects
the packaged app as the default terminal, starts a new CMD console and checks
input through the adopted session. It restores the account's previous registry
preferences and registrations afterward. It is separate from the regular CTest
suite.

Run preference, installation and GUI checks on their native OS before a release.
Linux and macOS behavior cannot be validated by running the Windows suite.
