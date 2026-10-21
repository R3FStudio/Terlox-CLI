#!/usr/bin/env python3
"""Select an installed Terlox for xdg-terminal-exec; retain other preferences."""
from pathlib import Path
import shutil
import subprocess

ENTRY = 'app.terlox.terminal.desktop'

def prefer(path):
    path.parent.mkdir(parents=True, exist_ok=True)
    previous = path.read_text() if path.exists() else ''
    lines = [line for line in previous.splitlines() if line.strip() != ENTRY]
    path.write_text(ENTRY + '\n' + '\n'.join(lines) + ('\n' if lines else ''))

def main():
    command = shutil.which('terloxctl')
    if not command:
        raise SystemExit('Install Terlox first, then run: terloxctl default-terminal set')
    raise SystemExit(subprocess.run([command, 'default-terminal', 'set']).returncode)

if __name__ == '__main__':
    main()
