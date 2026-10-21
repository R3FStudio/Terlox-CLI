# Install Terlox

[Docs home](../README.md)

Open the repository's **Releases** page and choose the download for your system.
You don't need a compiler, Python or Qt to use an installed release.

## Windows

1. Download the Windows setup `.exe`.
2. Open it and choose where to install Terlox.
3. Choose your shortcuts and whether to make Terlox the default terminal.
4. Review the choices and click **Install Terlox**.

Setup installs for your account. The suggested folder is
`%LOCALAPPDATA%\Programs\Terlox`. Start menu, Explorer and desktop shortcuts are
optional. You can change the default-terminal preference later.

To update, close Terlox and run the new installer into the same folder. Your
preferences and shell history are kept.

To uninstall, open **Settings > Apps > Installed apps**, find **Terlox** and
choose **Uninstall**. You can also run `Uninstall.exe` from its installation
folder. Close terminal sessions first. Personal settings, history and files you
added to that folder are kept.

## Linux

On Debian or Ubuntu, download the `.deb`, open a terminal in your Downloads
folder, and install it with APT:

```sh
sudo apt install ./terlox_*_amd64.deb
```

Keep only the package you want to install in that folder before running the
command. APT installs the app and adds its launcher to your application menu.
Install a newer `.deb` the same way when updating.

For a portable copy, download the AppImage. In its file properties, allow it to
run as a program, then open it. You can also mark the downloaded file executable
with `chmod +x` in a shell. Keep it in a permanent folder if you plan to use it
as your default terminal. To update, close Terlox and replace the AppImage.

## macOS

Download the `.pkg` for **Apple silicon** or **Intel**, open it, and follow macOS
Installer. You can check your processor under **Apple menu > About This Mac**.
The app is installed at `/Applications/Terlox.app`.

To update, quit Terlox and run the newer package. Preferences stay in your
account. To remove the app, move it from Applications to the Trash.

## Choose your default terminal

You can use these commands after installing:

```sh
terloxctl default-terminal set
terloxctl default-terminal status
terloxctl default-terminal restore
```

`set` selects Terlox, `status` checks the selection, and `restore` returns to
your previous choice. In Terlox's own shell, `terlox-default` offers the same
actions.

Each desktop handles default terminals differently. See
[Default terminal](../using/default-terminal.md) for the behavior on your OS.

If you're working on Terlox itself, follow
[Dependency setup](../development/dependencies.md) and
[Build from source](../development/building.md).
