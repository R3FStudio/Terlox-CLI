"""Deploy the pinned, signed Microsoft console runtime next to Windows binaries."""
import argparse
import importlib.util
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]

def deploy(destination):
    spec = importlib.util.spec_from_file_location('package_tools', ROOT/'scripts/Fetch-Package-Tools.py')
    tools = importlib.util.module_from_spec(spec); spec.loader.exec_module(tools)
    host = tools.fetch('conpty')
    runtime = host.parents[4]/'runtimes/win-x64/native/conpty.dll'
    destination.mkdir(parents=True, exist_ok=True)
    shutil.copy2(host, destination/'OpenConsole.exe')
    shutil.copy2(runtime, destination/'conpty.dll')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(); parser.add_argument('--destination', type=Path, required=True)
    deploy(parser.parse_args().destination.resolve())
