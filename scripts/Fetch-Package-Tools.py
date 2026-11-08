"""Fetch a pinned release compiler/deployer into .tools, never into the OS."""
from pathlib import Path
import hashlib
import http.client
import json
import sys
import time
import socket
import urllib.error
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def download_verified(url, checksum, attempts=5):
    """Retry transient upstream failures; never cache an incomplete download."""
    for attempt in range(attempts):
        try:
            request = urllib.request.Request(url, headers={'User-Agent': 'Terlox-package-tools/1.0'})
            with urllib.request.urlopen(request, timeout=90) as response:
                content = response.read()
            if hashlib.sha256(content).hexdigest() != checksum:
                raise RuntimeError('Package-tool checksum mismatch; refusing the download.')
            return content
        except urllib.error.HTTPError as error:
            if error.code not in (408, 429, 500, 502, 503, 504) or attempt+1 == attempts:
                raise
        except (urllib.error.URLError, TimeoutError, ConnectionError, socket.timeout, http.client.IncompleteRead):
            if attempt+1 == attempts:
                raise
        delay = min(2**attempt, 16)
        print(f'Package-tool download interrupted; retry {attempt+2}/{attempts} in {delay}s', file=sys.stderr)
        time.sleep(delay)


def fetch(name):
    tool = json.loads((ROOT/'installer/packaging/tools.json').read_text())[name]
    destination = ROOT/'.tools/package-tools'
    destination.mkdir(parents=True, exist_ok=True)
    download = destination/(name+'.zip' if tool.get('archive') == 'zip' else tool['executable'])
    if not download.exists() or hashlib.sha256(download.read_bytes()).hexdigest() != tool['sha256']:
        content = download_verified(tool['url'], tool['sha256'])
        temporary = download.with_suffix(download.suffix+'.part')
        temporary.write_bytes(content)
        temporary.replace(download)
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
