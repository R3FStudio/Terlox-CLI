# How updates work

[Docs home](../README.md) | [Using updates](../using/updates.md) | [Release packaging](releases.md)

The shell command and `terloxctl update` share `updates/Command.cpp`. Without
the `check` argument they ask for `yes` when a newer release is available.
`check` never installs anything. `/terlox update` opens a terminal session for
the same command so keyboard input reaches the confirmation prompt.

## From confirmation to restart

`updates/Service.cpp` runs inside the GUI process. Its local socket is restricted
to the current account, and requests need the random token inherited by that
window's shell sessions. The GUI starts the detached installer so closing a
terminal session's process job cannot kill the installer with it.

The service checks GitHub's latest stable release again, then
`updates/InstallPlan.cpp` identifies the installed package type and selects its
matching asset. `updates/Download.cpp` streams that asset into a temporary file
and compares its size and SHA-256 against the GitHub release asset metadata.
Missing metadata, interrupted downloads and checksum mismatches stop the update
before the running app closes. HTTPS redirects are limited to GitHub's release
download servers.

Only after verification does the service launch the installation worker and ask
the app to quit. The worker waits for the old GUI process to exit.

| Platform | Installation worker |
| --- | --- |
| Windows | The downloaded setup EXE accepts an internal `/UPDATE` request. `installer/maintenance/Update.cpp`, running from the extracted payload, updates the existing installation and starts its new executable. |
| Linux Debian | A detached shell worker runs `pkexec` and `apt-get` to install the local package. PolicyKit owns the permission prompt. |
| Linux AppImage | The worker copies into a temporary file beside the existing AppImage, sets executable permissions and renames it into place. A failed copy keeps the old file. |
| macOS | The worker uses AppleScript's administrator authentication and the system `installer` command to apply the PKG. |

The worker writes `result.json` in the update's temporary folder. On failure,
the restarted app shows that report; Unix workers also write `install.log`
there. Windows file replacement uses the installer's existing rollback path.
Windows cannot update files held by another Terlox instance, so those instances
should be closed first. This flow does not restore running shell jobs or tabs
after restarting; saved appearance and profile settings remain in place.

## Releasing an update

Use the normal [version and release process](versioning.md). Upload the actual
native installers as GitHub release assets. GitHub supplies each uploaded asset's
size and SHA-256 digest through its release API; the updater needs both. No extra
checksum ZIP or manifest needs to appear in the release downloads.

`tests/UpdateTests.cpp` covers confirmation, asset selection, corrupted downloads,
socket authentication and AppImage replacement/restart. The Windows install smoke
test also exercises the self-contained setup EXE's update mode against a temporary
installation, including waiting for the previous process and preserving user files.
