# Find your way through the code

[Docs home](../README.md) · [How the app works](architecture.md)

Each component has its own folder. A header (`.h`) describes an interface; its
`.cpp` file implements it. A
`CMakeLists.txt` tells the build system which files belong to that component.

Use this table to find the files for a change:

| Change | Start with |
| --- | --- |
| App launch options | `app/Application.cpp`, `app/LaunchArguments.cpp` |
| Right-click actions | `window/WindowActions.cpp` |
| Administrator session launch | `window/WindowAdmin.cpp`, `platform/AdminLaunch.cpp` |
| Text selection or clipboard | `window/TerminalInput.cpp`, `window/Selection.cpp` |
| Scrolling | `window/TerminalScroll.cpp` |
| Font defaults | `settings/fonts/TerminalFont.cpp`, `settings/Appearance.cpp` |
| Glass and the outline | `platform/*/Backdrop.*`, `window/FrameBorder.cpp` |
| Color or escape-sequence bugs | `terminal/Screen.cpp`, `terminal/Extensions.cpp` |
| A shell command behaving incorrectly | `shell/runtime`, then `shell/parsing` |
| Setup launch errors | `installer/windows/Bootstrap.cpp`, `scripts/Package-Windows.ps1` |

## `app` — application startup

[`main.cpp`](../../app/main.cpp) enters
[`Application.cpp`](../../app/Application.cpp). This prepares the environment
and graphics backend, creates Qt's application object, reads launch options and
opens the main window. [`LaunchArguments.cpp`](../../app/LaunchArguments.cpp)
separates `-e` and the child program's arguments so flags intended for the child
are not mistaken for Terlox options.

## `window` — what you interact with

| File | Job |
| --- | --- |
| `Window.cpp` / `.h` | Own tabs and sessions; arrange and paint the outer window |
| `WindowActions.cpp` | Keyboard actions and the animated right-click menu |
| `WindowAdmin.cpp` | Explicit administrator/root session action |
| `WindowFrame.cpp` | Blank-strip dragging, resize edges and top-level mouse behavior |
| `WindowCommands.cpp` | `/terlox` commands and profile changes |
| `WindowInformation.cpp` | Collect information for About/System |
| `TerminalView.cpp` / `.h` | The terminal widget, font metrics, grid size and refresh scheduling |
| `TerminalPaint.cpp` | Turn visible screen cells into a renderer frame |
| `TerminalInput.cpp` | Keyboard, mouse, paste and CLI input routing |
| `TerminalScroll.cpp` | History position, smooth scrolling and the right scrollbar |
| `Selection.cpp` | Selected text and clipboard operations |
| `TerminalAbout.cpp` | Open and navigate the isolated About screen |
| `ApplicationControls.cpp` | Apply temporary CLI opacity/scale controls |
| `FrameBorder.cpp` / `.h` | Draw the rose border and antialiased corners |
| `AnimatedMenu.cpp` / `.h` | Menu appearance and opening animation |
| `ZoomControls.cpp` / `.h` | The menu's minus, percentage, plus and reset row |
| `ProfileDialog.cpp` / `.h` | Edit a custom shell profile |

## `sessions` — one running terminal

[`Session`](../../sessions/Session.cpp) connects a native process to a terminal
screen. Process output feeds the screen; key bytes and terminal replies go back
to the process. A tab owns its view, and that view owns the session. Closing the
tab tears down the session.

## `terminal` — understand what programs print

| File | Job |
| --- | --- |
| `Screen.cpp` / `.h` | Wrap libvterm and expose cells, cursor and screen state |
| `Keyboard.cpp` | Translate keys into terminal input bytes |
| `Scrollback.cpp` / `.h` | Keep a bounded collection of old lines |
| `Sequences.cpp` / `.h` | Gather escape messages even when they arrive in fragments |
| `Extensions.cpp` | Themes, synchronized updates, mouse/focus messages and app controls |
| `Graphics.cpp` / `.h` | Decode and bound inline images and placements |
| `EditableInput.cpp` / `.h` | Map a terminal selection to the shell's editable command range |

A terminal program often prints invisible control sequences alongside text.
These files interpret those sequences. They do not decide which shell command
to execute.

## `renderer` — put the screen on the GPU

[`Frame.h`](../../renderer/Frame.h) is the handoff format: glyphs, colored
rectangles, images and a cursor. [`SceneItem.cpp`](../../renderer/SceneItem.cpp)
updates Qt scene-graph nodes from that frame.
[`GlyphCache.cpp`](../../renderer/GlyphCache.cpp) rasterizes a glyph when needed
and caches its texture for reuse. [`GraphicsBackend.cpp`](../../renderer/GraphicsBackend.cpp)
chooses the platform backend and reports the active renderer. Parsing and layout
remain CPU work; the renderer does not execute shell commands.

## `platform` — talk to the operating system

| Area | Job |
| --- | --- |
| `Process.h`, `LaunchOptions.h` | Common process contract and launch settings |
| `windows/ConPty.cpp` | Windows pipes, pseudo-console, process job and lifetime |
| `posix/Pty.cpp` | POSIX PTY creation, polling, resize and child lifetime |
| `Environment.cpp` | Preserve the inherited environment and add platform PATH locations |
| `Elevation.cpp` / `.h` | Build elevation requests and parse explicit admin commands |
| `AdminLaunch.cpp` / `.h` | Choose native elevated PowerShell/CMD or sudo shell arguments |
| `windows/Elevation.cpp` | Ask Windows for UAC through `runas` |
| `windows/Manifest.cpp` | Inspect an executable's requested privilege level without running it |
| `windows/CommandLine.cpp` | Quote Windows executable arguments correctly |
| `windows/Backdrop.cpp` | Native Windows glass and border integration |
| `macos/Backdrop.mm` | Cocoa behind-window vibrancy |
| `posix/Backdrop.cpp` | Linux backdrop fallback; no native blur request yet |

`.mm` is Objective-C++, used here to call macOS APIs from the C++ project.

## `shell` — the separate `terloxsh` program

[`shell/main.cpp`](../../shell/main.cpp) chooses interactive input, a script or
`-c` command execution. This executable uses QtCore and runs without the GUI.

| Folder | Files and responsibility |
| --- | --- |
| `parsing` | `Command.h` describes commands/pipelines. `Parser.cpp` reads quotes, separators and redirects. `Expansion.cpp` expands environment variables and exit status. |
| `runtime` | `Builtins.cpp` handles commands that must change the shell itself, such as `cd`. `Executor.cpp` starts processes and connects pipes. `NativeShell.cpp` bridges Windows native engines and explicit admin requests. |
| `editing` | `LineEditor.cpp` reads interactive keystrokes. `History.cpp` stores earlier commands. `Completion.cpp` suggests paths/commands. `Layout.cpp` accounts for wrapped prompts. |

For example, `cd` must run in the parent shell because a separate child cannot
change its parent's folder. A program such as Git runs as a real child process.

`editing/InputState.cpp` publishes the active command's text, cursor and cell
widths to the terminal. `window/Selection.cpp` uses that state to delete a selected
range through the shell's editing keys. `LineEditor.cpp` clears the state before
executing a command or leaving the prompt.

## `settings` — defaults and saved choices

`Appearance.cpp` defines the initial look. `Store.cpp` reads/writes user
preferences and migrates old defaults. `Profiles.cpp` discovers installed shells
and sets their starting folder. `Commands.cpp` validates appearance command
values. `AdminProfile.cpp` wraps a native admin launch as a session profile.

`fonts/TerminalFont.cpp` privately registers the bundled Cascadia faces;
`TerminalFonts.qrc` embeds the TTF files into the application. Changing a font
does not install anything into the OS.

## `installer` — get the built app onto a machine

| Folder/file | Job |
| --- | --- |
| `windows/Bootstrap.cpp` | Small Qt-free launcher; find a complete Windows package before starting the wizard |
| `windows/SelfExtract.nsi` | Embed the entire Windows runtime in the release EXE and manage temporary unpacking |
| `packaging/tools.json` | Pinned packaging-tool downloads and checksums |
| `main.cpp` | Start the Qt wizard or validate a package without installing |
| `model/InstallPlan.cpp` | Read package metadata, validate paths and decide which files may be written |
| `backend/InstallEngine.cpp` | Copy files, keep temporary backups, roll back copy failures and save inventory |
| `integration/DesktopIntegration.cpp` | Per-user shortcuts, registration and supported XDG default preference |
| `wizard/SetupPages.cpp` | Folder/options/review/progress pages |
| `wizard/SetupWizard.cpp` | Wizard styling, flow, final result and optional launch |

The build folder contains tests and SDK-linked binaries, so setup never treats
it as a release payload. The package marker identifies the deployed folder.

## The remaining folders

| Folder | What belongs there |
| --- | --- |
| `information` | Shared version, shortcut catalog, release history, text report and About page |
| `updates` | GitHub release lookup, version comparison, platform download selection and update commands |
| `branding` | Icon painter, exported PNG/ICO/ICNS files and shared Qt icon resources |
| `branding/presentation` | README screenshot tool and its background window |
| `integration` | Linux desktop metadata, Windows register/unregister scripts and macOS bundle/install files |
| `examples` | Python examples for text colors, appearance controls, images and the README screenshot |
| `tests` | C++ terminal/process/GUI/installer checks and Python metadata/docs checks |
| `scripts` | Build, single-EXE/native packaging, cleanup, contribution checks and package smoke tests |
| `cmake` | Build rules for the vendored terminal library |
| `dependencies` | Third-party code/fonts, local terminal-engine changes and license texts |
| `.github/workflows` | Native build/test/package jobs |
| `docs` | User guides, references and developer explanations |

`build*`, `dist` and `.tools` are generated output or local SDKs. They are ignored
by Git.

## Default-terminal integration

Windows uninstall support lives in `installer/uninstall` (inventory validation
and file removal) and `installer/windows` (Installed apps registration, running-file
checks, integration cleanup and repair). `Uninstaller.nsi` builds the native
uninstall UI. Its temporary Qt Core helper is `installer/maintenance/main.cpp`.

`integration/defaults` defines the account preference API and shared command
parser. `integration/cli/main.cpp` is `terloxctl`; the shell builtin calls the
same backend. Windows, Linux and macOS backends live in their matching folders.

On Windows, `HandoffServer.cpp` receives COM requests, while the `handoff` folder
contains the MIDL interfaces and proxy build. `platform/InboundSession.h` owns
received handles until the Windows process backend adopts them. Normal sessions
still create their own PTYs. `scripts/Deploy-Conpty.py` verifies and deploys the
pinned Microsoft runtime. The installer registers default-terminal integration
when that option is selected.

On macOS, `app/FileOpenHandler.cpp` turns a Launch Services `.command` open event
into a session. Linux writes a desktop entry and xdg-terminal-exec preferences.
