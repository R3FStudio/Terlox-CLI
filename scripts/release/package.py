"""Package locally, or ask the native GitHub runners to build every platform."""
import json
import os
import platform
import shutil
import sys
import time
import uuid
from .common import ROOT, run, assets


def local(args):
    system = platform.system()
    environment = dict(os.environ)
    if args.qt_root:
        from pathlib import Path
        qt_bin = Path(args.qt_root).resolve() / 'bin'
        environment['PATH'] = str(qt_bin) + os.pathsep + environment.get('PATH', '')
        environment['QMAKE'] = str(qt_bin / ('qmake6' if (qt_bin / 'qmake6').is_file() else 'qmake'))
    if system == 'Windows':
        qt = ['-QtRoot', args.qt_root] if args.qt_root else []
        prefix = ['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File']
        if not args.skip_build:
            run(prefix + ['scripts/Build-Windows.ps1', '-Configuration', 'Release', '-SkipTests'] + qt, dry_run=args.dry_run)
        command = prefix + ['scripts/Package-Windows.ps1'] + qt
    elif system in ('Linux', 'Darwin'):
        if not args.skip_build:
            command = ['cmake', '--preset', 'release']
            if args.qt_root:
                command += [f'-DCMAKE_PREFIX_PATH={args.qt_root}']
            run(command, dry_run=args.dry_run)
            run(['cmake', '--build', '--preset', 'release', '--parallel', '4'], dry_run=args.dry_run)
        name = 'Linux' if system == 'Linux' else 'macOS'
        command = [sys.executable, f'scripts/Package-{name}.py']
        if system == 'Darwin':
            for field in ('app_signing_identity', 'installer_signing_identity'):
                if getattr(args, field):
                    command += ['--' + field.replace('_', '-'), getattr(args, field)]
    else:
        raise SystemExit(f'No native packager for {system}.')
    run(command, dry_run=args.dry_run, env=environment)
    if args.check:
        name = 'Smoke-Package.py' if system == 'Windows' else 'Smoke-Unix-Packages.py'
        run([sys.executable, f'scripts/{name}'], dry_run=args.dry_run)


def all_platforms(args):
    # Correlate this dispatch with its run, even if another contributor builds too.
    request = uuid.uuid4().hex
    # --repo belongs to individual gh commands, not the gh root command.
    repo = ['--repo', args.repo] if args.repo else []
    dispatch = ['gh', 'workflow', 'run', 'build.yml', '--ref', args.ref,
                '-f', f'request_id={request}'] + repo
    run(dispatch, dry_run=args.dry_run)
    if args.dry_run:
        print('Then wait for that run and download its terlox-* installer artifacts into dist/.')
        return
    deadline = time.monotonic() + 180
    while time.monotonic() < deadline:
        runs = json.loads(run(['gh', 'run', 'list', '--workflow', 'build.yml',
                              '--event', 'workflow_dispatch', '--limit', '50',
                              '--json', 'databaseId,displayTitle'] + repo, capture=True))
        match = next((r for r in runs if r['displayTitle'] == f'Packages {request}'), None)
        if match:
            break
        time.sleep(5)
    else:
        raise SystemExit('The workflow was dispatched, but its run was not found. Check GitHub Actions.')
    identifier = str(match['databaseId'])
    run(['gh', 'run', 'watch', identifier, '--exit-status'] + repo)
    destination = ROOT / 'build-release' / 'release-downloads' / request
    run(['gh', 'run', 'download', identifier, '--pattern', 'terlox-*',
         '--dir', str(destination)] + repo)
    release = ROOT / 'dist'
    release.mkdir(exist_ok=True)
    found = [p for p in destination.rglob('*') if p.is_file()
             and p.suffix in ('.exe', '.deb', '.AppImage', '.pkg')]
    if sorted(p.name for p in found) != sorted(p.name for p in assets('all')):
        raise SystemExit(f'The downloaded files do not match this checkout\'s version. Inspect {destination}.')
    for asset in found:
        shutil.copy2(asset, release / asset.name)
    print(f'All five installers are in {release}. Nothing has been published.')
