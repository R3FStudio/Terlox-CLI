# Package and release Terlox

[Docs home](../README.md) | [Build from source](building.md) | [Installer internals](installer.md)

Run commands from the project root. Start with [dependency setup](dependencies.md)
if you need the build tools or Qt. Packages are written to `dist/`.

## Package on your own machine

The same command works on each supported OS:

```sh
python scripts/Package.py --check
```

It builds a release, calls that OS's packager, then checks the packaged runtime.
If you already built the app, add `--skip-build`. Use `--qt-root PATH` when Qt is
not in the usual local folder or on PATH. Run `--dry-run` to see the commands.

| Build machine | Output |
| --- | --- |
| Windows x64 | One self-contained `Terlox-VERSION-windows-x64-setup.exe` |
| Linux x64 | `terlox_VERSION_amd64.deb` and `Terlox-VERSION-x86_64.AppImage` |
| Apple silicon Mac | `Terlox-VERSION-macos-arm64.pkg` |
| Intel Mac | `Terlox-VERSION-macos-x64.pkg` |

This builds for your current OS. To build the other platforms as well, use
the all-platform command below.

The lower-level scripts are still available: `Package-Windows.ps1`,
`Package-Linux.py` and `Package-macOS.py`. They deploy Qt, copy the license texts,
and call NSIS, linuxdeploy/dpkg, or Apple's `pkgbuild`. The Windows script also
fetches the checksum-pinned packaging tools and console host. Nothing from the
internal `build-release/package` staging folders belongs beside the download.

## Package every platform

Push the branch you want to build, sign into GitHub CLI with `gh auth login`,
then run:

```sh
python scripts/Package.py --all --ref main
```

Replace `main` with your pushed branch or version tag. Add `--repo OWNER/REPO`
if this checkout does not identify the intended GitHub repository.

The command starts `.github/workflows/build.yml`, waits for its native Windows,
Linux, Apple silicon and Intel jobs, then downloads their five installers into
`dist/`. Each job builds, tests and checks its packages. A failed job stops the
command before it copies release files. It does not publish a release.

GitHub builds the pushed ref, not uncommitted files on your computer. The
updated workflow and these scripts must be present on that ref. You need Actions
enabled and permission to dispatch the workflow. You can also start **Native
builds** from GitHub's Actions page and download each `terlox-*` artifact yourself.

## Choose the version

Change `project(Terlox VERSION ...)` in `CMakeLists.txt`, then add a short entry
in `information/Releases.cpp`. CMake supplies the app version and platform
metadata; the packagers read the same value for filenames. Rebuild after changing
it. Use the same version for the Git tag, with a `v` prefix.

See [Change the project version](versioning.md) for the file locations, generated
metadata and checks to run when making this change yourself.

Before tagging a release, run the native CTest suite and documentation checks:

```sh
ctest --test-dir build-release --output-on-failure
python tests/Check-Docs.py
python tests/ReleaseCommandsTests.py
```

Also try installation and an upgrade in a test account. On Windows,
`python tests/WindowsUninstallSmoke.py` checks a temporary install, uninstall
repair and native removal. The opt-in `WindowsDefaultTerminal.py` check changes
and restores the test account's terminal preference; see
[Test default-terminal integration](building.md#test-default-terminal-integration)
before running it.

## Upload a release

Commit the version change, create a matching Git tag with a `v` prefix, and
push the tag. The release commands read the version from `CMakeLists.txt`; you
do not need to type it again in the upload commands.

Build packages from that tag. Upload all five with:

```sh
python scripts/Publish.py --platform all
```

The command checks the expected filenames, requires the tag to exist on GitHub,
creates a draft if there is no release yet, and uploads the files. It refuses
to overwrite an existing asset. Review the release notes and binaries on GitHub
before making the draft public. Adding `--publish` publishes after uploading.
If the release is already public, uploading adds files directly to that release.

You can upload each platform separately to the same draft:

```sh
python scripts/Publish.py --platform windows
python scripts/Publish.py --platform linux
python scripts/Publish.py --platform macos-arm64
python scripts/Publish.py --platform macos-x64
```

`--platform macos` uploads both Mac packages together. `--repo OWNER/REPO` selects
a repository; `--dry-run` prints the commands without uploading. These commands
publish GitHub release assets, not submissions to Microsoft Store, Homebrew or a
Debian repository. Those distribution channels have their own submission process.
The script uses the official [GitHub release commands](https://cli.github.com/manual/gh_release).

## Signing and macOS notarization

Windows signing needs your own certificate and signing process. Keep keys and
certificates outside the repository.

On macOS you can pass Developer ID identities through the packaging command:

```sh
python scripts/Package.py --app-signing-identity "Developer ID Application: YOUR IDENTITY" --installer-signing-identity "Developer ID Installer: YOUR IDENTITY"
```

Submit the resulting `.pkg` using your configured keychain profile:

```sh
PACKAGE="/path/to/the-built.pkg"
xcrun notarytool submit "$PACKAGE" --keychain-profile Terlox --wait
xcrun stapler staple "$PACKAGE"
```

Set `PACKAGE` to the file you just built. Check acceptance before stapling, and
follow
[Apple's notarization instructions](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution)
when setting up the profile. The all-platform workflow currently produces
unsigned packages; it does not have your signing credentials.

## Updating an installed app

The [in-terminal updater](../using/updates.md) can download, install and restart
after confirmation. For a manual upgrade, use the steps below.

**Windows:** close Terlox and run the new setup EXE into the existing folder. The
inventory lets setup replace app files while keeping preferences and user-added
files. Uninstall is available from Windows Installed apps or `Uninstall.exe`.

**Linux:** install the newer `.deb` with the package manager, or replace your
AppImage and keep its launcher pointing to the new file.

**macOS:** quit Terlox and run the newer `.pkg`. It replaces
`/Applications/Terlox.app`. Remove any separate development copy under your home
folder if you keep opening the wrong version.

For old build output on Windows, run `scripts/Clean-Workspace.ps1` to preview
what it would remove, then add `-Apply` to clean it. It keeps current release
files, source, documentation images and local SDKs. Close programs running from
obsolete build folders before applying the cleanup.
