"""Inspect native packages and start their deployed application without SDK paths."""
from pathlib import Path
import os
import platform
import re
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
VERSION=re.search(r'project\(Terlox VERSION ([0-9.]+)',(ROOT/'CMakeLists.txt').read_text()).group(1)


def run(*args,**options):
    return subprocess.run([str(arg) for arg in args],check=True,timeout=120,**options)


environment=dict(os.environ)
for key in ['LD_LIBRARY_PATH','DYLD_LIBRARY_PATH','QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QML2_IMPORT_PATH']:
    environment.pop(key,None)
environment.update(QT_QUICK_BACKEND='software',QT_QPA_PLATFORM='offscreen')
with tempfile.TemporaryDirectory(prefix='terlox-package-check-') as temporary:
    temporary=Path(temporary)
    if platform.system()=='Linux':
        appimage=ROOT/'dist'/f'Terlox-{VERSION}-x86_64.AppImage'
        deb=ROOT/'dist'/f'terlox_{VERSION}_amd64.deb'
        environment['APPIMAGE_EXTRACT_AND_RUN']='1'
        report=temporary/'appimage.txt'
        run(appimage,'--diagnostics','--diagnostics-output',report,env=environment)
        assert f'TERLOX {VERSION}' in report.read_text()
        fields=run('dpkg-deb','-f',deb,'Package','Version','Architecture',capture_output=True,text=True).stdout
        assert 'terlox' in fields and VERSION in fields and 'amd64' in fields,fields
        extracted=temporary/'deb'; run('dpkg-deb','-x',deb,extracted)
        # AppRun resolves its bundled libraries from its own folder. Test the
        # unpacked payload without installing anything into the CI runner.
        report=temporary/'deb.txt'
        run(extracted/'opt/terlox/AppRun','--diagnostics','--diagnostics-output',report,env=environment)
        assert f'TERLOX {VERSION}' in report.read_text()
        preference=run(extracted/'opt/terlox/usr/bin/terloxctl','default-terminal','status',env=environment,capture_output=True,text=True)
        assert 'xdg-terminal-exec' in preference.stdout
    elif platform.system()=='Darwin':
        architecture='arm64' if platform.machine()=='arm64' else 'x64'
        installer=ROOT/'dist'/f'Terlox-{VERSION}-macos-{architecture}.pkg'
        expanded=temporary/'expanded'; run('pkgutil','--expand-full',installer,expanded)
        apps=list(expanded.rglob('Terlox.app')); assert len(apps)==1,apps
        app=apps[0]; report=temporary/'macos.txt'
        assert (app/'Contents/MacOS/terloxsh').is_file()
        preference=run(app/'Contents/MacOS/terloxctl','default-terminal','status',env=environment,capture_output=True,text=True)
        assert 'handler' in preference.stdout
        assert (app/'Contents/Frameworks').is_dir()
        run(app/'Contents/MacOS/terlox','--diagnostics','--diagnostics-output',report,env=environment)
        assert f'TERLOX {VERSION}' in report.read_text()
    else:
        raise SystemExit('Run the package smoke test on Linux or macOS.')
print('Native package metadata, bundled runtime and application startup passed.')
