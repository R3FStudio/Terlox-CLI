"""Build a bundled AppImage and a Debian package on native Linux x86_64."""
import argparse
from pathlib import Path
import importlib.util
import os
import platform
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def run(*args, **options):
    return subprocess.run([str(a) for a in args],check=True,**options)


def package(build):
    if platform.system() != 'Linux' or platform.machine() not in ('x86_64','amd64'):
        raise SystemExit('Run this packager on Linux x86_64, or use the Linux CI job.')
    for command in ['dpkg-deb','dpkg-shlibdeps','qmake6']:
        if not shutil.which(command) and command != 'qmake6':
            raise SystemExit(f'Install the native packaging tool: {command}')
    spec = importlib.util.spec_from_file_location('package_tools',ROOT/'scripts/Fetch-Package-Tools.py')
    tools = importlib.util.module_from_spec(spec); spec.loader.exec_module(tools)
    deploy = tools.fetch('linuxdeploy'); tools.fetch('linuxdeploy-qt')
    version = re.search(r'project\(Terlox VERSION ([0-9.]+)',(ROOT/'CMakeLists.txt').read_text()).group(1)
    work = build/'package/linux-x64'; appdir = work/'Terlox.AppDir'
    if work.exists():
        if not work.resolve().is_relative_to(build.resolve()): raise RuntimeError('Unsafe staging folder')
        shutil.rmtree(work)
    appdir.mkdir(parents=True); release = ROOT/'dist'; release.mkdir(exist_ok=True)
    run('cmake','--install',build,'--prefix',appdir/'usr')
    # The native package manager installs the app; the cross-platform developer
    # wizard is not part of Linux releases.
    for name in ['terlox-setup','terlox-setup-wizard']:
        (appdir/'usr/bin'/name).unlink(missing_ok=True)
    share = appdir/'usr/share/terlox'
    shutil.copytree(ROOT/'docs',share/'docs',dirs_exist_ok=True)
    shutil.copytree(ROOT/'dependencies/licenses/qt',share/'licenses/qt',dirs_exist_ok=True)
    for name in ['LICENSE','NOTICE','README.md','CONTRIBUTING.md']:
        shutil.copy2(ROOT/name,share/name)
    environment = dict(os.environ)
    environment['PATH'] = str(deploy.parent)+os.pathsep+environment['PATH']
    environment['APPIMAGE_EXTRACT_AND_RUN'] = '1'
    environment['QMAKE'] = os.environ.get('QMAKE') or shutil.which('qmake6') or shutil.which('qmake') or ''
    if not environment['QMAKE']: raise SystemExit('Put Qt 6.8+ qmake on PATH, or set QMAKE.')
    environment['OUTPUT'] = str(release/f'Terlox-{version}-x86_64.AppImage')
    environment['VERSION'] = version
    # The same private runtime is also checked headlessly in CI.
    plugins=Path(run(environment['QMAKE'],'-query','QT_INSTALL_PLUGINS',capture_output=True,text=True).stdout.strip())
    offscreen=plugins/'platforms/libqoffscreen.so'
    if offscreen.is_file():
        target=appdir/'usr/plugins/platforms'; target.mkdir(parents=True,exist_ok=True); shutil.copy2(offscreen,target)
    run(deploy,'--appdir',appdir,'--executable',appdir/'usr/bin/terlox',
        '--executable',appdir/'usr/bin/terloxsh',
        '--executable',appdir/'usr/bin/terloxctl',
        '--desktop-file',ROOT/'integration/linux/app.terlox.terminal.desktop',
        '--icon-file',appdir/'usr/share/icons/hicolor/256x256/apps/app.terlox.terminal.png',
        '--plugin','qt','--output','appimage',env=environment,cwd=work)
    # Reuse the same deployed runtime inside the native Debian package.
    deb = work/'deb'; opt = deb/'opt/terlox'; opt.parent.mkdir(parents=True)
    shutil.copytree(appdir,opt,symlinks=True)
    binaries = deb/'usr/bin'; binaries.mkdir(parents=True)
    (binaries/'terlox').write_text('#!/bin/sh\nexec /opt/terlox/AppRun "$@"\n')
    (binaries/'terloxsh').write_text('#!/bin/sh\nexport LD_LIBRARY_PATH="/opt/terlox/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\nexec /opt/terlox/usr/bin/terloxsh "$@"\n')
    (binaries/'terloxctl').write_text('#!/bin/sh\nexport LD_LIBRARY_PATH="/opt/terlox/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\nexec /opt/terlox/usr/bin/terloxctl "$@"\n')
    for binary in binaries.iterdir(): binary.chmod(0o755)
    applications = deb/'usr/share/applications'; applications.mkdir(parents=True)
    shutil.copy2(ROOT/'integration/linux/app.terlox.terminal.desktop',applications)
    icon = deb/'usr/share/icons/hicolor/256x256/apps'; icon.mkdir(parents=True)
    shutil.copy2(ROOT/'branding/icons/terlox-256.png',icon/'app.terlox.terminal.png')
    metadata = deb/'DEBIAN'; metadata.mkdir()
    # Calculate system-library dependencies while treating bundled Qt libraries
    # as private files, rather than requiring an incompatible distro Qt release.
    dependency_work = work/'dependency-check'; (dependency_work/'debian').mkdir(parents=True)
    (dependency_work/'debian/control').write_text('Source: terlox\nSection: utils\nPriority: optional\nMaintainer: R3F Studio\n\nPackage: terlox\nArchitecture: amd64\nDescription: Cross-platform terminal and shell\n')
    # Plugins and bundled libraries can depend on system libraries which the
    # two executables never link directly (for example XCB and font libraries).
    # Include every deployed ELF file so apt installs those dependencies too.
    elf_files = set()
    for deployed in opt.rglob('*'):
        if deployed.is_file():
            with deployed.open('rb') as stream:
                if stream.read(4) == b'\x7fELF':
                    elf_files.add(deployed.resolve())
    command = ['dpkg-shlibdeps','--ignore-missing-info','-O',f'-l{opt/"usr/lib"}']
    command.extend(f'-e{binary}' for binary in sorted(elf_files))
    dependencies = run(*command,cwd=dependency_work,capture_output=True,text=True).stdout
    depends = next((line.split('=',1)[1] for line in dependencies.splitlines() if line.startswith('shlibs:Depends=')),None)
    if not depends:
        raise RuntimeError('dpkg-shlibdeps did not report the runtime dependencies')
    size = sum(p.stat().st_size for p in deb.rglob('*') if p.is_file())//1024
    (metadata/'control').write_text(f'Package: terlox\nVersion: {version}\nSection: utils\nPriority: optional\nArchitecture: amd64\nMaintainer: R3F Studio\nInstalled-Size: {size}\nDepends: {depends}\nDescription: Cross-platform terminal with its own shell\n Native terminal sessions, bundled Cascadia Mono, and terloxsh.\n')
    run('dpkg-deb','--root-owner-group','--build',deb,release/f'terlox_{version}_amd64.deb')
    return version


if __name__ == '__main__':
    parser=argparse.ArgumentParser(); parser.add_argument('--build',type=Path,default=ROOT/'build-release')
    args=parser.parse_args(); package(args.build.resolve())
