# Update Terlox

[Command guide](command-guide.md)

In the included Terlox shell, run:

```text
terlox-update
```

From PowerShell, CMD, Bash or another shell, use:

```text
terloxctl update
```

You can also type `/terlox update` at a fresh prompt inside the Terlox window.
When a newer stable release is available, the command asks whether to update.
Type `yes` and press Enter to download it, close Terlox's sessions, install it
and reopen the app. Finish or save any work in those sessions before confirming.
Any other answer cancels the update.

The checker contacts the
[Terlox GitHub repository](https://github.com/R3FStudio/Terlox-CLI), compares
the latest public stable release with the installed version, and shows the
release page. When an update is available, it includes matching download links
for your OS and architecture. Drafts and prereleases are excluded.

The download's size and SHA-256 checksum are checked before Terlox closes.
A failed download leaves the current installation running. Installation keeps
your saved settings. If installation fails or you cancel the system permission
prompt, Terlox reopens and shows the error report.

| Installation | What happens after confirmation |
| --- | --- |
| Windows setup EXE | Setup updates the existing installation folder and restarts Terlox. |
| Linux Debian package | The desktop asks for permission through PolicyKit, then `apt-get` installs the downloaded package. |
| Linux AppImage | The new AppImage replaces the existing file. The file and its folder must be writable. |
| macOS PKG | macOS asks for administrator permission, installs the package and reopens Terlox from Applications. |

Automatic installation runs from an installed Terlox window. A source build or
a manually copied app may need the [normal installer](../getting-started/install.md)
first. Linux Debian updates need `apt-get`, `pkexec` and a working desktop
authentication agent. macOS updates expect the PKG-installed copy in Applications.
Close other Terlox instances before updating on Windows so their files are free.

## Check without installing

```text
terlox-update check
terloxctl update check
```

These commands show the available release and download links without asking to
install. `/terlox update check` does the same inside the app. They are suitable
for scripts or for checking from another terminal.

Older builds that only show download links need one manual installer upgrade
to gain the confirmation-and-restart flow.

If there is no public release yet, the command says so. A private repository,
network problem or GitHub rate limit can also prevent a check. You can always
open the [Releases page](https://github.com/R3FStudio/Terlox-CLI/releases)
yourself. The checker doesn't require GitHub CLI or sign you into an account.
