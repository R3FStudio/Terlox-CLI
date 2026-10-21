#include "Report.h"
#include "Shortcuts.h"
#include "Releases.h"
#include "information/Version.h"
#include <QSysInfo>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
namespace terlox {
QString informationReport(const ReportContext& c) {
    QString text = QString("TERLOX %1\nNative terminal and independent shell\n\n").arg(TERLOX_VERSION);
    text += "THIS COMPUTER\n";
    text += "OS: " + QSysInfo::prettyProductName() + "\nKernel: " + QSysInfo::kernelType() + " " + QSysInfo::kernelVersion() + "\n";
    text += "CPU/build: " + QSysInfo::currentCpuArchitecture() + " / " + QSysInfo::buildCpuArchitecture() + "\n";
    text += QString("Qt runtime: %1; compiled with: %2\n").arg(qVersion(),QT_VERSION_STR);
    text += "Renderer: " + (c.renderer.isEmpty() ? qEnvironmentVariable("TERLOX_RENDERER", "host-dependent; press F1 in the GUI for the active device") : c.renderer) + "\n";
    text += "Backdrop: " + (c.backdrop.isEmpty() ? QString("actual compositor transparency; black alpha tint; no background image") : c.backdrop) + "\n";
    if (!c.profile.isEmpty()) text += "Profile: " + c.profile + "\n";
    if (!c.executable.isEmpty()) text += "Executable: " + c.executable + "\n";
    if (!c.arguments.isEmpty()) text += "Launch arguments: " + c.arguments.join(" | ") + "\n";
    text += c.directory.isEmpty() ? "Working directory: " + QDir::currentPath() + "\n" : "Launch directory: " + c.directory + " (query the shell for its current directory)\n";
    if (!c.surface.isEmpty()) text += "Terminal surface: " + c.surface + "\n";
    if (!c.profiles.isEmpty()) text += "Available profiles: " + c.profiles.join(", ") + "\n";
    text += "\nKEYBOARD SHORTCUTS (native names for this OS)\n";
    for (const auto& s : shortcuts()) {
        auto keys = s.keys;
#ifdef Q_OS_MACOS
        keys.replace("Ctrl","Cmd").replace("Alt","Option");
#endif
        text += keys.leftJustified(24) + s.description + "\n";
    }
    text += "Ctrl+A                 Select all retained terminal text\n";
    text += "Ctrl+C / Ctrl+V        Copy selection / paste; Ctrl+C interrupts if nothing selected\n";
    text += "Ctrl+Alt+A/C/V         Send raw Ctrl+A/C/V to a CLI application\n";
    text += "Ctrl+wheel / Shift +/-  Temporary text zoom (6-40 pt; resets on close)\n";
    text += "Shift+PageUp/PageDown    Scroll terminal history\n";
    text += "Drag blank top strip    Move window; cursor unchanged; double-click has no window action\n";
    text += "Terminal content       Text/blank/Alt drag select; never move window\n";
    text += "Shift+left-drag        Select text in mouse-aware CLI apps\n";
    text += "Right-click zoom row   Minus / percentage / plus / reset arrow\n";
    text += "Window edges/corners   Resize window\nRight-click            Animated session / window menu\n";
    text += "\nCOMMANDS AND SCRIPTS\n";
    text += "About Terlox            Interactive in-terminal About app (1-4 pages, Q exits)\n/info-terlox            Opens the same app; standalone prints this report\n";
    text += "terloxsh -c 'command'   Run a command without the GUI\nterloxsh file.tlx       Run a line-based Terlox script\n";
    text += "terloxctl default-terminal set/status/restore  Manage this account's OS terminal preference\n";
    text += "terlox-default [set|status|restore]  Same actions inside terloxsh\n";
    text += "terlox-update [check]    Check the latest public stable release\n";
    text += "terloxctl update        Same update check from any shell\n";
    text += "terlox --set-default-terminal  GUI executable command-line equivalent\n";
    text += "Builtins: cd, pwd, echo, clear, exit, export/set, unset, env, history\n";
    text += "Quotes: single literal, double expanded; $NAME, ${NAME}, $?\nPipes: a | b; redirection: <, >, >>, 2>; statements: ;\n";
    text += "Editor: arrows/history, Tab completion, Home/End, Ctrl+A/E/L, Ctrl+C/D\n";
    text += "\nWINDOWS\n";
    text += "Default Windows directory: %SystemRoot%/System32 (explicit profile/launch directories override).\n";
    text += "Native ConPTY sessions for Windows PowerShell, PowerShell 7, CMD and custom shells.\n";
    text += "Select a shell in New session; its complete native language runs there.\n";
    text += "In terloxsh: ps Get-ChildItem | Select-Object -First 5\n";
    text += "             cmd dir /b\n             powershell -File script.ps1\n";
    text += "Direct .ps1/.cmd/.bat invocation uses the appropriate installed interpreter.\n";
    text += "Unresolved Verb-Noun cmdlets and common CMD builtins route to their native engine.\n";
    text += "PowerShell aliases such as ls/cat/cp/mv/rm work when no PATH executable overrides them.\n";
    text += "cd /d, chdir, sl and Set-Location persist cwd; the prompt shows its current path.\n";
    text += "Windows .com/.cmd/.bat commands resolve via PATH and PATHEXT.\n";
    text += "Right-click > Run as administrator > PowerShell / Command Prompt opens a UAC-approved window.\n";
    text += "Ordinary sessions stay unelevated. The Terlox shell requests UAC for standalone executables requiring elevation.\n";
    text += "Use sudo --cmd command or sudo --powershell command to request elevation explicitly.\n";
    text += "/admin [command] works in any GUI profile and uses that profile launch directory.\n";
    text += "UAC cancellation runs nothing; no elevation occurs without OS consent.\n";
    text += "Script execution policy, installed modules and permissions remain OS controlled.\n";
    text += "GPU default: Direct3D 11; per-pixel compositor transparency with a black tint.\n";
    text += "Windows uses D3D11/OpenGL; Vulkan/D3D12 overrides fall back to D3D11 for alpha.\n";
    text += "\nLINUX\n";
    text += "Native PTYs; Bash, Zsh, Fish and custom installed shells.\n";
    text += "Run sudo/su as ordinary external programs; their native TTY handles passwords.\n";
    text += "Right-click > Open root session (sudo) opens a native shell with in-terminal authentication.\n";
    text += "System-shell profiles provide native shell syntax and script execution.\n";
    text += "GPU default: OpenGL; Vulkan can be selected with QSG_RHI_BACKEND=vulkan.\n";
    text += "Real alpha transparency requires a compositing window manager.\n";
    text += "\nMACOS\n";
    text += "Native PTYs; Zsh, Bash, Fish and custom installed shells; native sudo and scripts.\n";
    text += "Right-click > Open administrator session (sudo) uses native sudo authentication.\n";
    text += "GPU default: Metal; Cocoa behind-window vibrancy where available.\n";
    text += "Qt displays Command/Option key equivalents in the shortcut list on macOS.\n";
    text += "\nRENDERING AND SETTINGS\n";
    text += "GPU scene-graph draws cached glyph textures, cell colors, selection and cursor.\n";
    text += "CPU performs process I/O, VT parsing, layout and initial glyph rasterization.\n";
    text += "QSG_RHI_BACKEND selects an installed RHI backend; QT_QUICK_BACKEND=software\n";
    text += "is an explicit software fallback for incompatible drivers/headless testing.\n";
    text += "Use /terlox help for appearance/profile commands; customization is absent from the menu.\nFont, blur, effects, opacity, border, scrollback and profiles persist via QSettings.\n";
    text += "\nINSTALLATION AND DEFAULTS\n";
    text += "terlox-setup provides a native per-user installation wizard from an extracted release.\n";
    text += "Linux desktop entry: TerminalEmulator; -e argv; xdg-terminal-exec preferences.\n";
    text += "Windows: per-user Start/Run and Explorer registration scripts; console-default delegation is not implemented.\n";
    text += "macOS: native application bundle; there is no universal system default-terminal preference.\n";
    text += "See docs/getting-started/install.md for installation and platform limits.\n";
    text += "\nENVIRONMENT AND COMMANDS\n";
    text += "Exported environment and PATH are inherited by sessions and their child processes.\n";
    text += "Windows supplements inherited PATH with registered user/system paths; inherited overrides take priority.\n";
    text += "macOS includes /etc/paths, /etc/paths.d and installed Homebrew paths.\n";
    text += "Use a native shell profile for its aliases, functions and startup scripts.\n";
    text += "A slim right scrollbar appears for retained history; wheel scrolling is animated.\n";
    text += "\nVERSION HISTORY\n" + releaseHistory();
    text += "\nCURRENT LIMITS\n";
    text += "Windows is locally verified; Linux/macOS code requires native validation.\n";
    text += "No POSIX job control or complete scripting language in terloxsh; choose a system shell.\n";
    text += "Mouse/focus/paste, RGB palette/theme OSCs and bounded inline bitmap graphics are supported.\n";
    text += "OSC 777 app controls provide transient opacity (0.2..1) and text scale (0.5..3).\n";
    text += "Windows ConPTY strips Kitty APC; inline PNG works. See docs/reference/cli-protocols.md.\n";
    text += "Sixel, complete Kitty graphics/keyboard, full IME preedit and universal CLI compatibility remain incomplete.\n";
    text += "No cross-platform performance benchmarks or universal GPU/blur guarantee.\n";
    text += "\nLICENSES\nTerlox: Apache License 2.0 (Apache-2.0). libvterm: MIT.\nCascadia Mono: Microsoft Corporation, SIL OFL-1.1.\nQt: its own LGPL/GPL/commercial terms. See LICENSE, NOTICE and third-party notices.\n";
    return text;
}
}
