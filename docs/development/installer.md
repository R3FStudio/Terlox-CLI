# Working on the installer

[Docs home](../README.md) · [Release commands](releases.md)

If you change a checkbox in setup, start in `installer/wizard`. If setup refuses
a folder or misses a file, start in `installer/model` or `installer/backend`.
The pages collect choices; the backend decides what those choices mean on disk.

## What runs when someone opens setup

The Windows download is a single EXE. `installer/windows/SelfExtract.nsi` packs
the app, Qt libraries, wizard and uninstaller into it. Opening it extracts that
payload into a temporary folder and starts `terlox-setup.exe`. That small launcher
checks the package before starting the Qt wizard. When the wizard closes, the
outer installer removes the extracted copy.

There are two setup executables in the build folder because the launcher must
be able to report missing runtime files without loading Qt first. Users only
download the outer EXE; they do not need either build-folder executable beside it.

Linux releases use `.deb` and AppImage. macOS uses `.pkg`. Those releases do not
run the Qt wizard: the native package tools install the app. Keep changes to
Windows wizard pages separate from Linux/macOS package scripts.

## Follow one installation

1. `SetupWizard.cpp` creates the welcome, destination, integration, review,
   install and finish pages. `options()` collects their values into `InstallOptions`.
2. `SetupPages.cpp` suggests a destination and checks the choices before moving
   forward. Windows defaults to `%LOCALAPPDATA%\Programs\Terlox`, so an ordinary
   account can install there without elevating the whole installer.
3. `model/InstallPlan.cpp` reads `.terlox-package.json`, checks the OS and payload,
   and lists the files to copy. It resolves paths before checking them. It also
   checks the previous installation inventory so an upgrade can distinguish app
   files from files the user added.
4. The review page shows that plan. The install page passes it to
   `backend/InstallEngine.cpp`, which copies files and keeps backups for copy
   failures. The resulting `.terlox-install.json` records the files Terlox owns.
5. Windows registration adds the app to Installed apps. Optional integration
   creates shortcuts and Explorer entries; the default-terminal choice calls the
   same backend as `terloxctl default-terminal set`.
6. The finish page shows the result and can launch the installed executable.
   Integration errors appear in the result notes.

The progress page runs copying in a worker so the window can keep repainting.
Cancel is disabled during installation. File copying and registry operations
belong in the plan and backend, which are shared with the maintenance tool and
tests.

## Add or change an option

For a new shortcut option, add the control and field in `SetupPages.cpp`, carry
it through `SetupWizard::options()` and `InstallOptions`, then implement its
effect in the integration backend. Update the review page to describe the option's
effect before installation.

Folder validation belongs in `buildInstallPlan()`, which is also called by the
maintenance command.
A destination that does not exist yet is normal: validate its existing parent
and create the missing folders during installation.

For styling, use `SetupWizard.cpp`. It owns the palette, spacing, logo and button
appearance. The terminal's frameless glass window is unrelated to this window.

## Updates and removal

An upgrade uses the old inventory to recognize files it may replace. The new
inventory keeps ownership of older runtime files so uninstall can remove them
later. Preferences and shell history live in the user's account, outside that
inventory.

`windows/Uninstaller.nsi` supplies the native uninstall window. It runs
`installer/maintenance/main.cpp` from a temporary runtime so the installed EXEs
and DLLs can be removed. `uninstall/UninstallPlan.cpp` validates the inventory;
`uninstall/UninstallEngine.cpp` removes those files and empty directories.
It leaves files the user added. `windows/RemoveIntegration.cpp` removes entries
only when they still refer to this installation.

The running-file check asks the user to close Terlox; it does not terminate
terminal sessions. Keep that behavior when changing upgrades or removal.

## Check a change

Build first, then run:

```sh
ctest --test-dir build-release --output-on-failure -R "installer|setup-wizard"
```

`tests/SetupWizardTests.cpp` checks navigation, review and the suggested folder.
`tests/InstallerTests.cpp` covers paths, copy failures, upgrades and inventory
removal. On Windows, package the result and run:

```sh
python scripts/Package.py --skip-build --check
python tests/WindowsUninstallSmoke.py
```

The last check performs a temporary installation, repairs legacy uninstall
support, and runs the native uninstaller. When changing wizard pages, also check
layout and navigation in a test account.
