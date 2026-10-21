"""Fetch a pinned release compiler/deployer into .tools, never into the OS."""
from pathlib import Path
import hashlib
import json
import sys
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def fetch(name):
    tool = json.loads((ROOT/'installer/packaging/tools.json').read_text())[name]
    destination = ROOT/'.tools/package-tools'
    destination.mkdir(parents=True, exist_ok=True)
    download = destination/(name+'.zip' if tool.get('archive') == 'zip' else tool['executable'])
    if not download.exists() or hashlib.sha256(download.read_bytes()).hexdigest() != tool['sha256']:
        with urllib.request.urlopen(tool['url'], timeout=90) as response:
            content = response.read()
        if hashlib.sha256(content).hexdigest() != tool['sha256']:
            raise RuntimeError(f'Checksum mismatch for {name}; package tools were not changed.')
        download.write_bytes(content)
    executable = destination/tool['executable']
    if tool.get('archive') == 'zip' and not executable.exists():
        with zipfile.ZipFile(download) as archive:
            for entry in archive.infolist():
                target = (destination/entry.filename).resolve()
                if not target.is_relative_to(destination.resolve()):
                    raise RuntimeError('Unsafe package-tool archive path')
            archive.extractall(destination/name if name == "conpty" else destination)
    executable.chmod(executable.stat().st_mode | 0o111)
    return executable


if __name__ == '__main__':
    print(fetch(sys.argv[1]))
