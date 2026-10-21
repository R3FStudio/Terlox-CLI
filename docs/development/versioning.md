# Change the project version

[Docs home](../README.md) | [Build from source](building.md) | [Package and release](releases.md)

The app version is set in the root [CMakeLists.txt](../../CMakeLists.txt), in
the `project(Terlox VERSION ...)` line near the top. Change the number there
when preparing a release. Use three numeric parts: `MAJOR.MINOR.PATCH`, replacing
each word with a number. Keep the existing form of the line; packaging scripts
read it directly. The current packaging expects numeric versions without a
prerelease suffix.

## Files to edit

| File | What to change |
| --- | --- |
| [CMakeLists.txt](../../CMakeLists.txt) | Replace the number after `VERSION` in the `project(Terlox ...)` declaration. This controls the app and package version on every platform. |
| [information/Releases.cpp](../../information/Releases.cpp) | Add an entry at the beginning of `releaseHistory()` describing the new release. Keep the older entries. This is the history shown inside Terlox; it does not set the app version. |

Use a patch change for fixes, a minor change for compatible new features, and a
major change for incompatible changes. Choose the number before building the
release so the app, installers and release tag agree.

## Where that value goes

These files consume the version automatically. Leave their placeholders in place.

| File | Used for |
| --- | --- |
| [information/Version.h.in](../../information/Version.h.in) | CMake generates the version header used by the app, About page, information report and shell. |
| [branding/Version.rc.in](../../branding/Version.rc.in) | Windows executable file and product version metadata. |
| [installer/windows/SelfExtract.nsi](../../installer/windows/SelfExtract.nsi) and [Uninstaller.nsi](../../installer/windows/Uninstaller.nsi) | Windows setup and uninstaller metadata, supplied by the packaging script. |
| [integration/macos/Info.plist.in](../../integration/macos/Info.plist.in) | macOS bundle version metadata. |
| [scripts/Package-Windows.ps1](../../scripts/Package-Windows.ps1), [Package-Linux.py](../../scripts/Package-Linux.py) and [Package-macOS.py](../../scripts/Package-macOS.py) | Installer filenames, package metadata and staging folders. |
| [scripts/release/common.py](../../scripts/release/common.py) | Version and expected filenames used by packaging and publishing commands. |

The `version` field in `CMakePresets.json`, the CMake minimum version, Qt/tool
versions and the `Version` field in the Linux desktop launcher describe other
formats or dependencies. Leave them alone when changing the Terlox release number.

## Rebuild and check

Run these commands from the project root after saving your edits:

```sh
python scripts/Package.py --check
ctest --test-dir build-release --output-on-failure
python tests/Check-Docs.py
python tests/ReleaseCommandsTests.py
```

Use `python3` if that is your Python command. See [dependency setup](dependencies.md)
if the compiler or Qt is not configured. The packaging command rebuilds for your
current OS and checks the resulting package. Avoid `--skip-build` immediately
after changing the version: old binaries can still contain the previous number.

Open the rebuilt app and check its About page, or run the rebuilt `terloxsh`
with `--version` (`build-release/bin/terloxsh.exe` on Windows and
`build-release/bin/terloxsh` on Linux and macOS). Check that the new installer
filenames in `dist/` use the same number. Rebuilding does not update an already
installed copy; install the new package to update that copy.

## Tag and publish

Commit the source changes, then create a Git tag using the same version with a
`v` prefix. Push the commit and tag before asking GitHub Actions to build it.
For an all-platform release, run `python scripts/Package.py --all --ref TAG`,
replacing `TAG` with that tag. This builds the pushed source on native runners.

Follow [Package and release](releases.md#upload-a-release) to upload the packages
and publish the GitHub release. The publishing command reads the version from
your checkout, so keep it on the release commit while publishing its packages.
Ordinary documentation does not need a version-number update for each release.
