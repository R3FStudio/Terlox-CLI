# Platform support

[Docs home](../README.md)

Terlox has native terminal and window backends for Windows, Linux and macOS.
The table below lists the platform-specific behavior.

| | Windows | Linux | macOS |
| --- | --- | --- | --- |
| Shell connection | ConPTY | POSIX PTY | POSIX PTY |
| Default GPU backend | Direct3D 11 | OpenGL | Metal |
| Transparent surface | Native compositor | Requires a compositor | Cocoa |
| Blur implementation | Native accent material | No built-in blur backend yet | Behind-window vibrancy |
| Privileged session | Separate UAC-approved window | New sudo shell tab | New sudo shell tab |
| Packaging | Single setup EXE with embedded runtime | Bundled-runtime `.deb` and AppImage | Native `.pkg` with deployed app bundle |

## Windows

ConPTY requires Windows 10 version 1809 or later. The current Windows build uses
a Windows 11 SDK. Native material effects depend on the OS and can fall back to
plain transparency. The accent-material call is dynamically resolved and uses
an undocumented Windows API and may vary between Windows releases.

Direct3D 11 is the default. Windows OpenGL can be selected, but Vulkan/D3D12
requests fall back to D3D11 for alpha transparency. The graphics driver and Qt
backend still determine what is available.

ConPTY transports ordinary terminal input/output and filters some escape
sequences. On the tested system it strips Kitty's graphics APC messages;
Terlox therefore uses inline PNG for its Windows image example.

Windows 11 or Windows 10 22H2 build 19045.3031 or newer supports per-user console delegation into Terlox.
Setup can select it, or use `terloxctl default-terminal set`. Handoff adopts the
original console session and its native program; it does not relaunch a guessed
command. Administrator sessions still follow Windows elevation rules. The
classic COM registration does not add an MSIX entry to Windows Terminal's
packaged-provider dropdown. See [Default terminal](../using/default-terminal.md).

## Linux

The process backend uses a real PTY. Sudo, su and installed commands use their
normal permissions. The window needs a compositor for transparency. The current
Linux backdrop implementation does not request a desktop-specific blur effect;
a compositor may apply its own rules.

The default preference targets `xdg-terminal-exec`, not every desktop's private
terminal setting. Linux packagers produce AppImage and Debian formats with a
private Qt runtime; compatible system libraries are still required.

## macOS

The native backdrop uses Cocoa vibrancy and the renderer defaults to Metal.
Supported installed shells are discovered through PATH. Native shell profiles
use their normal interactive/login startup behavior.

The deployed app contains its shell under `Contents/MacOS`; Qt frameworks must
also be deployed into the bundle. There is no universal macOS terminal-default
setting. The preference command selects Terlox for terminal `.command` files
through Launch Services. Editors and Finder services can have their own choice.

## CLI compatibility

Terlox supports common terminal behavior, RGB colors, alternate screens, mouse
and focus reporting, bracketed paste and bounded inline images. It does not yet
implement every xterm, Kitty or Sixel feature, full IME preedit, complete
grapheme editing or universal CLI compatibility. See the
[application reference](cli-protocols.md) before depending on an extension.
