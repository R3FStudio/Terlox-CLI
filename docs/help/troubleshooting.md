# Something isn't working

[Docs home](../README.md)

## Setup says a Qt DLL is missing

Use the Windows setup EXE from the repository's Releases page. If setup reports
unpacking trouble, check the free space in your temporary folder and download
a fresh copy. If you're building from source, use the packaging command in
[Build from source](../development/building.md).

Do not download individual DLLs from a DLL download site. The package already
contains the matching Qt runtime. If you only have a build tree, follow
[Build and test](../development/building.md).

## Setup says it cannot overwrite a file

Close all installed Terlox windows and shell processes, then retry with the
setup EXE. For a fresh installation, choose an empty dedicated
folder. Setup will not overwrite an unrelated file that happens to have the
same name. A previous Terlox installation is recognized by its install manifest.

You can use the suggested installation path unchanged. If it is unavailable,
check that your account can write to its parent folder.

## A command is missing or behaves differently

First check **New session** and choose the shell the command was written for.
PowerShell functions and modules need PowerShell; CMD scripts need CMD; a Bash
script needs Bash. Choose a native shell profile for its scripting language.

If an executable such as Git is missing, check that it is installed and on PATH.
Open a new session after changing PATH. [More about shells](../using/sessions-and-commands.md).

## DiskPart or another tool needs administrator rights

The included Windows shell requests UAC for a standalone program whose manifest
requires elevation, including DiskPart. For other commands, use
`sudo --cmd command`, `/admin --cmd command`, or the right-click administrator
action. On Linux/macOS, use your native sudo/su tool inside the terminal.
[Administrator commands](../using/administrator-commands.md) explains password
input and the OS's authentication policy.

## Text is too small, too large or looks crowded

Right-click and use the zoom row's reset arrow. To reset the saved font itself:

```text
/terlox font "Cascadia Mono" 12
```

Cascadia Mono is bundled. Proportional fonts can produce uneven character
spacing in a terminal grid. [Font settings](../using/appearance.md).

## I can't select text or open the menu in a CLI app

The program may have enabled mouse input. Hold **Shift** while selecting, or
**Shift + right-click** for Terlox's menu. Use Ctrl+Alt+A/C/V when the program
needs the raw Ctrl shortcut rather than terminal selection or clipboard behavior.

## The background doesn't look like the README

Transparency shows what is behind your window. Your desktop, compositor, tint
settings and the background colors printed by applications affect the result.
Blur also varies by OS. Try lowering `/terlox opacity` or moving a window behind
Terlox. [Platform notes](../reference/platforms.md).

## A program doesn't show its animation or progress bar

Run it directly in an interactive session first. Saving or piping its output
can make the program hide its live display. Color and progress preferences in
your shell or the program's own configuration can also disable it.

For npm, `npm config get progress` checks its setting. Use
`npm install --progress=true` to request progress for an installation. npm also
checks whether its output streams are terminals.
[npm's configuration reference](https://docs.npmjs.com/cli/using-npm/config/#progress)
describes that behavior.

In a PowerShell session, `$ProgressPreference = 'Continue'` enables progress
records if a profile or script had suppressed them. See
[PowerShell's progress preference](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_preference_variables#progresspreference).

`NO_COLOR` can disable text color effects. Remove it from the current session
if you want those effects: `unset NO_COLOR` in Bash/Zsh/sh,
`Remove-Item Env:NO_COLOR -ErrorAction SilentlyContinue` in PowerShell,
`set NO_COLOR=` in CMD, or `set -e NO_COLOR` in Fish.

New Terlox sessions advertise true color and a terminal connection. Explicit
color preferences inherited from the launching shell, such as `NO_COLOR`, are
kept. Check those variables if you launched Terlox from another command runner.

## The window is blank or the GPU is failing

For diagnosis, launch Terlox with `QT_QUICK_BACKEND=software` in the environment.
This selects software rendering. If that works, check the graphics driver and
the reported renderer in **F1 > System**.
The developer guide explains the [rendering path](../development/architecture.md).

## What information helps with a bug report?

Include the Terlox version, OS, selected shell, the exact command and its error
text. F1's System page contains the active renderer and launch information.
Remove personal paths, credentials and other private output before sharing a
screenshot or diagnostic report.
