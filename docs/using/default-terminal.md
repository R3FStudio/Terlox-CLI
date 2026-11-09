# Default terminal

[Docs home](../README.md) · [Installation](../getting-started/install.md)

The preference belongs to your account. It does not elevate Terlox or change
the shell you use inside it.

## Commands

Run these ordinary commands from PowerShell, CMD or a Unix shell such as Bash,
including a shell running inside Terlox:

```sh
terloxctl default-terminal set
terloxctl default-terminal status
terloxctl default-terminal restore
```

`set` registers the installed app and saves the previous preference. Repeating
it does not replace that backup with Terlox itself. `restore` returns the saved
preference if Terlox is still selected; it leaves a newer selection alone.

In Terlox's own shell, use `terlox-default set`, `terlox-default status` or
`terlox-default restore`. The GUI executable also accepts `--set-default-terminal`,
`--default-terminal-status` and `--restore-default-terminal`. Use `terloxctl`
when you want a normal console command with visible output and an exit code.

## Windows

Windows 11 or Windows 10 22H2 build 19045.3031 or newer is required for this handoff implementation. Select
**Use Terlox as the default terminal** during setup, or run the command afterward.
The installer leaves the checkbox unchecked by default. It adds the installation
folder to your user PATH; open a new shell if an older shell cannot find `terloxctl`.

New console applications can then open inside Terlox. Their original arguments,
working directory and environment remain attached to that console session.
Existing sessions do not move to Terlox.

Terlox does not appear in Windows Terminal's packaged-provider dropdown.
Use the command's `status` action to check the selection. Windows can retain its
own handling for elevated or otherwise excluded console launches. UAC still
applies to commands that require administrator rights.

Before moving or removing an installation, restore the previous preference. Run
`set` from the new installed location after moving it. Leave installed runtime
files together.

## Linux

The command writes a Terlox desktop entry in your user applications directory
and puts it first in the generic and current-desktop `xdg-terminals.list` files.
Existing entries and comments stay in the list. Install `xdg-terminal-exec` if
your distribution does not already provide it. Applications that use that
launcher honor this preference; an editor or desktop with a private terminal
setting can require a separate choice.

For an AppImage, you can rename the downloaded file to `Terlox.AppImage`, then run:

```sh
./Terlox.AppImage --set-default-terminal
```

The preference uses the AppImage's saved location. Run the command again if you
move the AppImage.

### Debian / Ubuntu (.deb installation)

On Debian and Ubuntu, the `x-terminal-emulator` system alternative is a separate
setting from the per-user `xdg-terminal-exec` preference. To configure both after
installing the Terlox `.deb`, run:

```sh
terloxctl default-terminal set
sudo apt install xdg-terminal-exec
sudo update-alternatives --install /usr/bin/x-terminal-emulator x-terminal-emulator /usr/bin/terlox 50
sudo update-alternatives --set x-terminal-emulator /usr/bin/terlox
```

Run each command on its own line. The alternative commands use the `.deb`'s
installed `/usr/bin/terlox` launcher and change the system-wide selection.

If `set` prints **“Terlox preference saved. Install xdg-terminal-exec for
applications that use it”**, the preference was saved successfully. Install the
launcher and configure the separate Debian alternative using the remaining
commands above. Applications with their own terminal setting may still need a
separate choice.

Test Debian's selection with:

```sh
x-terminal-emulator
```

It should open Terlox. **Terminal emulator** is the standard name for a terminal
application; it does not mean emulating another operating system.
`x-terminal-emulator` is Debian's generic launcher for the selected terminal app.

To choose another system terminal later, run:

```sh
sudo update-alternatives --config x-terminal-emulator
```

`terloxctl default-terminal restore` restores the per-user launcher preference;
it does not undo a system alternative selected with `sudo`. The per-user command
never runs sudo or changes system alternatives implicitly.

## macOS

The native package installs `/Applications/Terlox.app` and command launchers
under `/usr/local/bin`. Run `terloxctl default-terminal set` after installation.
For a manually copied app, the bundled command is also available directly:

```sh
/Applications/Terlox.app/Contents/MacOS/terloxctl default-terminal set
```

This registers the app with Launch Services and selects it as the shell handler
for executable terminal `.command` files. Opening such a file creates a Terlox
session in its folder. Terlox does not make a non-executable script executable.
macOS may ask you to confirm an application change.

macOS has no Windows-style switch that redirects every console launch. Finder's
terminal services, editors and other launchers can retain their own settings.
Choose Terlox in those tools when they offer an external-terminal preference.

