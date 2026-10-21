"""Create a native macOS installer package with deployed Qt frameworks."""
import argparse
from pathlib import Path
import platform
import re
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]


def run(*args):
    subprocess.run([str(arg) for arg in args],check=True)


def package(build, app_identity='', installer_identity=''):
    if platform.system()!='Darwin': raise SystemExit('Run this packager on macOS, or use the macOS CI jobs.')
    version=re.search(r'project\(Terlox VERSION ([0-9.]+)',(ROOT/'CMakeLists.txt').read_text()).group(1)
    architecture='arm64' if platform.machine()=='arm64' else 'x64'
    work=build/'package'/f'macos-{architecture}'; payload=work/'root'
    if work.exists():
        if not work.resolve().is_relative_to(build.resolve()): raise RuntimeError('Unsafe staging folder')
        shutil.rmtree(work)
    app=payload/'Applications/Terlox.app'; app.parent.mkdir(parents=True)
    run('/usr/bin/ditto',build/'bin/terlox.app',app)
    resources=app/'Contents/Resources'; resources.mkdir(exist_ok=True)
    for name in ['LICENSE','NOTICE','README.md','CONTRIBUTING.md']: shutil.copy2(ROOT/name,resources/name)
    shutil.copytree(ROOT/'docs',resources/'docs',dirs_exist_ok=True)
    licenses=resources/'licenses'; licenses.mkdir(exist_ok=True)
    shutil.copytree(ROOT/'dependencies/licenses/qt',licenses/'qt',dirs_exist_ok=True)
    shutil.copy2(ROOT/'dependencies/libvterm/LICENSE',licenses/'libvterm-MIT.txt')
    shutil.copy2(ROOT/'dependencies/fonts/cascadia/LICENSE',licenses/'Cascadia-OFL-1.1.txt')
    qmake=shutil.which('qmake6') or shutil.which('qmake')
    if not qmake: raise SystemExit('Put the Qt 6.8+ bin directory on PATH.')
    plugin_root=Path(subprocess.run([qmake,'-query','QT_INSTALL_PLUGINS'],check=True,capture_output=True,text=True).stdout.strip())
    offscreen=app/'Contents/PlugIns/platforms/libqoffscreen.dylib'; offscreen.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(plugin_root/'platforms/libqoffscreen.dylib',offscreen)
    command=['macdeployqt',str(app),f'-executable={app/"Contents/MacOS/terloxsh"}',f'-executable={app/"Contents/MacOS/terloxctl"}',f'-executable={offscreen}','-always-overwrite']
    if app_identity: command += [f'-sign-for-notarization={app_identity}']
    run(*command)
    # Native package-owned command launchers; no shell profile edits required.
    binaries=payload/'usr/local/bin'; binaries.mkdir(parents=True)
    for name in ['terlox','terloxsh','terloxctl']:
        launcher=binaries/name
        launcher.write_text(f'#!/bin/sh\nexec /Applications/Terlox.app/Contents/MacOS/{name} "$@"\n')
        launcher.chmod(0o755)
    release=ROOT/'dist'; release.mkdir(exist_ok=True)
    target=release/f'Terlox-{version}-macos-{architecture}.pkg'
    command=['pkgbuild','--root',str(payload),'--identifier','app.terlox.terminal','--version',version,'--install-location','/']
    if installer_identity: command+=['--sign',installer_identity]
    run(*command,str(target))
    return target


if __name__=='__main__':
    parser=argparse.ArgumentParser(); parser.add_argument('--build',type=Path,default=ROOT/'build-release')
    parser.add_argument('--app-signing-identity',default=''); parser.add_argument('--installer-signing-identity',default='')
    args=parser.parse_args(); print(package(args.build.resolve(),args.app_signing_identity,args.installer_signing_identity))
