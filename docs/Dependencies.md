# Third-party notices

[Docs home](README.md) | [Install build dependencies](development/dependencies.md)

Terlox's original code, documentation and artwork use the
[Apache License 2.0](../LICENSE). The components below keep their own licenses.

| Component | Used for | License and source |
| --- | --- | --- |
| Qt | Windows, input, fonts and GPU scene graph | LGPL/GPL/commercial terms; [Qt licensing](https://doc.qt.io/qt-6/licensing.html) |
| libvterm | Parse terminal output and maintain the screen | MIT; [included license](../dependencies/libvterm/LICENSE) |
| Cascadia Mono | Bundled terminal font | SIL Open Font License 1.1; [included license](../dependencies/fonts/cascadia/LICENSE) |
| NSIS | Self-extracting Windows installer | zlib/libpng stub/compression; [upstream terms](https://nsis.sourceforge.io/License) |
| Microsoft ConPTY | Signed Windows console host and handoff runtime | MIT; [included notice](../dependencies/licenses/Microsoft-Terminal-MIT.txt) |

## Qt

Qt is dynamically linked. The Windows package carries its DLLs, runtime plugins,
license texts and the SDK's software bill of materials. Preserve those notices
when distributing a build. Compatible Qt libraries can be replaced independently
of Terlox's executable.

Qt's source archives are available from the official
[Qt archive](https://download.qt.io/archive/qt/). Use the release matching the
SDK used to build the package.
The full Qt terms and any corresponding-source requirements still apply to a
binary distribution; Terlox's Apache license does not replace them.

## libvterm

The vendored library is built statically. Its upstream implementation lives in
`dependencies/libvterm/engine`, with headers under `include`. Provenance is
recorded in
[`dependencies/README.md`](../dependencies/README.md).

## Cascadia Mono

Regular, Bold, Italic and BoldItalic static TTF files are embedded unchanged.
They are registered privately inside the app, not installed into the OS font
directory. Microsoft retains the copyright and reserved font name.
The [font provenance file](../dependencies/fonts/cascadia/README.md) records the
official release and archive checksum.

Packages include `libvterm-MIT.txt`, `Cascadia-OFL-1.1.txt` and Qt license texts.
The root [NOTICE](../NOTICE) identifies these dependencies as well.

The Windows payload also includes `NSIS-COPYING.txt`. The installer uses NSIS's
zlib compressor; its other optional compression modules are not used. Packaging
tools are pinned by URL/checksum in `installer/packaging/tools.json` and cached
under `.tools/package-tools`. They are build tools, not terminal dependencies.

Windows also bundles Microsoft's signed `OpenConsole.exe` and `conpty.dll` from
the pinned Microsoft.Windows.Console.ConPTY NuGet package. The COM interface
definitions under `integration/windows/handoff` come from Microsoft Terminal
under MIT. Their copyright and license are retained in
`Microsoft-Terminal-MIT.txt`. Terlox's private proxy DLL is built from those
interfaces; it does not modify Microsoft's binaries.
