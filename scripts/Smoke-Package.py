"""Verify deployed executables without inheriting a Qt SDK search path."""
import os
from pathlib import Path
import subprocess
import tempfile
import re
import argparse

parser=argparse.ArgumentParser()
parser.add_argument('--software',action='store_true',help='Use software/offscreen rendering on a headless CI runner')
options=parser.parse_args()

root = Path(__file__).resolve().parents[1]
version = re.search(r"project\(Terlox VERSION ([0-9.]+)", (root / "CMakeLists.txt").read_text()).group(1)
package = root / "build-release" / "package" / f"windows-x64-{version}"
environment = dict(os.environ)
environment["PATH"] = str(Path(os.environ["SystemRoot"]) / "System32")
for key in ("QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH", "QTDIR", "QT_QUICK_BACKEND", "QSG_RHI_BACKEND", "QSG_RHI_PREFER_SOFTWARE_RENDERER"):
    environment.pop(key, None)
if options.software:
    environment.update(QT_QUICK_BACKEND='software',QT_QPA_PLATFORM='offscreen')
with tempfile.TemporaryDirectory(prefix="terlox-package-") as working:
    preference = subprocess.run([str(package/'terloxctl.exe'),'default-terminal','status'],env=environment,capture_output=True,timeout=10)
    assert preference.returncode==0 and b'default terminal' in preference.stdout,(preference.returncode,preference.stderr)
    import shutil
    version = re.search(r"project\(Terlox VERSION ([0-9.]+)", (root / "CMakeLists.txt").read_text()).group(1)
    installer = root/'dist'/f'Terlox-{version}-windows-x64-setup.exe'
    # Only this EXE is copied. All runtime files must come from its payload.
    standalone = Path(working)/'single installer'; standalone.mkdir()
    single_exe = standalone/'Terlox-setup.exe'; shutil.copy2(installer,single_exe)
    verification = subprocess.run([str(single_exe), '/VERIFY'],cwd=standalone,env=environment,capture_output=True,timeout=120)
    assert verification.returncode == 0, (verification.returncode,verification.stderr)
    assert sorted(p.name for p in standalone.iterdir()) == ['Terlox-setup.exe']
    setup = subprocess.run(
        [str(package/'terlox-setup.exe'), '--check-package', '--destination', str(Path(working)/'installed')],
        cwd=working, env=environment, capture_output=True, timeout=15,
    )
    assert setup.returncode == 0, (setup.returncode, setup.stderr)
    # Reproduce opening the build/bin setup shown in the reported DLL errors.
    raw_setup = subprocess.run(
        [str(root/'build-release/bin/terlox-setup.exe'), '--check-package', '--destination', str(Path(working)/'raw-installed')],
        cwd=working, env=environment, capture_output=True, timeout=15,
    )
    assert raw_setup.returncode == 0, (raw_setup.returncode, raw_setup.stderr)
    # A detached launcher fails clearly, without Windows missing-DLL dialogs.
    import shutil
    isolated = Path(working)/'isolated'; isolated.mkdir()
    shutil.copy2(package/'terlox-setup.exe', isolated/'terlox-setup.exe')
    absent = subprocess.run([str(isolated/'terlox-setup.exe'), '--check-package'], env=environment,capture_output=True,timeout=10)
    assert absent.returncode == 2, (absent.returncode,absent.stderr)
    assert b'complete Terlox installer folder' in absent.stderr
    # Read embedded Windows icon resources, without launching or registering anything.
    import ctypes
    extract = ctypes.windll.shell32.ExtractIconExW
    extract.argtypes = [ctypes.c_wchar_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint]
    for executable in ['terlox.exe','terloxsh.exe','terlox-setup.exe']:
        assert extract(str(package/executable), -1, None, None, 0) >= 1, executable
    shell = subprocess.run(
        [str(package / "terloxsh.exe"), "-c", "echo portable-ok | findstr portable > proof.txt"],
        cwd=working, env=environment, capture_output=True, timeout=10,
    )
    assert shell.returncode == 0, (shell.returncode, shell.stderr)
    assert Path(working, "proof.txt").read_text().strip() == "portable-ok"
    listing = subprocess.run(
        [str(package / "terloxsh.exe"), "-c", "ls | Select-Object -ExpandProperty Name"],
        cwd=working, env=environment, capture_output=True, timeout=15,
    )
    assert listing.returncode == 0, (listing.returncode, listing.stderr)
    assert b"proof.txt" in listing.stdout, listing.stdout
    gui = subprocess.run(
        [str(package / "terlox.exe"), "--version"],
        cwd=working, env=environment, capture_output=True, timeout=10,
    )
    assert gui.returncode == 0, (gui.returncode, gui.stderr)
    diagnostics = Path(working, "diagnostics.txt")
    startup = subprocess.run(
        [str(package / "terlox.exe"), "--profile", "Terlox shell", "--diagnostics", "--diagnostics-output", str(diagnostics)],
        cwd=working, env=environment, capture_output=True, timeout=15,
    )
    assert startup.returncode == 0, (startup.returncode, startup.stderr)
    report = diagnostics.read_text(encoding="utf-8")
    assert ("Software scene graph" if options.software else "Qt RHI scene graph") in report, report
    version = re.search(r"project\(Terlox VERSION ([0-9.]+)", (root / "CMakeLists.txt").read_text()).group(1)
    assert f"TERLOX {version}" in report, report
    # Execute a real child with spaced/quoted arguments; preserve argv after -e.
    launch_proof = Path(working, 'launch arguments.txt')
    command = "import pathlib,sys; pathlib.Path(sys.argv[1]).write_text('|'.join(sys.argv[2:]))"
    import sys
    process = subprocess.Popen(
        [str(package/'terlox.exe'), '--working-directory', working, '-e', sys.executable,
         '-c', command, str(launch_proof), 'two words', '--profile', 'quote"inside', 'trailing\\'],
        cwd=working, env=environment,
    )
    try:
        import time
        until = time.monotonic()+10
        while not launch_proof.exists() and time.monotonic()<until:
            time.sleep(.05)
        assert launch_proof.read_text() == 'two words|--profile|quote"inside|trailing\\'
    finally:
        process.terminate()
        process.wait(timeout=5)
    missing_program = subprocess.run([str(package/'terlox.exe'), '-e'], env=environment, capture_output=True, timeout=10)
    assert missing_program.returncode == 2, missing_program.returncode
print("Single-file installer, developer launcher, native commands, GPU frame, argument handling and icons passed without SDK PATH.")
