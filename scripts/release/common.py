"""Shared paths and process handling for release commands."""
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def version():
    return re.search(r'project\(Terlox VERSION ([0-9.]+)',
                     (ROOT / 'CMakeLists.txt').read_text()).group(1)


def run(command, *, dry_run=False, capture=False, env=None):
    command = [str(part) for part in command]
    print(shlex.join(command), flush=True)
    if dry_run:
        return ''
    result = subprocess.run(command, cwd=ROOT, check=True, text=True,
                            capture_output=capture, env=env)
    return result.stdout if capture else ''


def assets(platform):
    v = version()
    names = {
        'windows': [f'Terlox-{v}-windows-x64-setup.exe'],
        'linux': [f'terlox_{v}_amd64.deb', f'Terlox-{v}-x86_64.AppImage'],
        'macos': [f'Terlox-{v}-macos-arm64.pkg', f'Terlox-{v}-macos-x64.pkg'],
    }
    return [ROOT / 'dist' / name for system, files in names.items()
            if platform in ('all', system) or (system == 'macos' and platform.startswith('macos-'))
            for name in files if not platform.startswith('macos-') or name.endswith(f'-{platform}.pkg')]
