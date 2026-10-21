"""Install and remove a temporary Windows package; restore account metadata."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import time
import winreg
import json
import hashlib
import re
from WindowsDefaultTerminal import KEYS, TERMINAL, snapshot, restore, normalized

ARP=r'Software\Microsoft\Windows\CurrentVersion\Uninstall\Terlox'

def main():
    root=Path(__file__).resolve().parents[1]
    version=re.search(r'project\(Terlox VERSION ([0-9.]+)',(root/'CMakeLists.txt').read_text()).group(1)
    parser=argparse.ArgumentParser(); parser.add_argument('--package',type=Path,default=root/'build-release/package'/f'windows-x64-{version}')
    package=parser.parse_args().package.resolve()
    keys=[*KEYS,ARP,r'Environment']
    saved={key:snapshot(key) for key in keys}
    environment=dict(os.environ); environment['PATH']=str(Path(os.environ['SystemRoot'])/'System32')
    try:
        with tempfile.TemporaryDirectory(prefix='terlox-uninstall-') as temporary:
            target=Path(temporary)/'installed app'
            installed=subprocess.run([str(package/'terlox-maintenance.exe'),'--install','--package',str(package),'--directory',str(target)],env=environment,capture_output=True,text=True,timeout=60)
            assert installed.returncode==0,(installed.stdout,installed.stderr)
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER,ARP) as registration:
                assert winreg.QueryValueEx(registration,'DisplayName')[0]=='Terlox'
                assert winreg.QueryValueEx(registration,'UninstallString')[0]==f'"{target / "Uninstall.exe"}"'
                assert winreg.QueryValueEx(registration,'QuietUninstallString')[0]==f'"{target / "Uninstall.exe"}" /S'
            print('Installed apps registration and included uninstaller verified, with desktop integration disabled.')
            # Reproduce an older installation that had no uninstaller or ARP
            # entry. Repair must not replace the working terminal executable.
            digest=hashlib.sha256((target/'terlox.exe').read_bytes()).digest()
            manifest=target/'.terlox-install.json'; inventory=json.loads(manifest.read_text())
            for name in ['Uninstall.exe','terlox-maintenance.exe']:
                (target/name).unlink(); inventory['files'].remove(name)
            manifest.write_text(json.dumps(inventory))
            restore(ARP,None)
            repaired=subprocess.run([str(package/'terlox-maintenance.exe'),'--repair-uninstaller','--package',str(package),'--directory',str(target)],env=environment,capture_output=True,text=True,timeout=30)
            assert repaired.returncode==0,(repaired.stdout,repaired.stderr)
            assert hashlib.sha256((target/'terlox.exe').read_bytes()).digest()==digest
            assert (target/'Uninstall.exe').is_file() and snapshot(ARP)
            print('Legacy installation repair added uninstall support without replacing the terminal.')
            personal=target/'my files/keep.txt'; personal.parent.mkdir(); personal.write_text('keep my work')
            # A separate baseline tests restore even when the user's installed
            # Terlox is already their default. The final block restores it.
            if saved[KEYS[0]] and any(name=='DelegationTerminal' and str(value).casefold()==TERMINAL.casefold() for name,value,_ in saved[KEYS[0]][0]):
                with winreg.OpenKey(winreg.HKEY_CURRENT_USER,KEYS[0],0,winreg.KEY_SET_VALUE) as key:
                    for name in ['DelegationTerminal','DelegationConsole']:
                        try: winreg.DeleteValue(key,name)
                        except FileNotFoundError: pass
            expected=snapshot(KEYS[0])
            selected=subprocess.run([str(target/'terloxctl.exe'),'default-terminal','set'],env=environment,capture_output=True,text=True,timeout=35)
            if selected.returncode!=0:
                assert 'Windows default-terminal handoff requires' in selected.stderr,(selected.stdout,selected.stderr)
                print('This Windows build does not support default-terminal handoff; checking uninstall without that option.')
            # A real user PATH entry must disappear without affecting others.
            with winreg.CreateKey(winreg.HKEY_CURRENT_USER,r'Environment') as key:
                try: previous=winreg.QueryValueEx(key,'Path')[0]
                except FileNotFoundError: previous=''
                winreg.SetValueEx(key,'Path',0,winreg.REG_EXPAND_SZ,previous+';'+str(target))
            # NSIS copies itself to temp; its initial process can exit before
            # the real uninstaller. Wait on its observable completion instead.
            launched=subprocess.Popen([str(target/'Uninstall.exe'),'/S'],env=environment)
            launched.wait(timeout=15)
            deadline=time.monotonic()+45
            while time.monotonic()<deadline and (target/'.terlox-install.json').exists(): time.sleep(0.1)
            assert not (target/'.terlox-install.json').exists(),'The native uninstaller did not finish.'
            assert not (target/'terlox.exe').exists()
            assert not (target/'Uninstall.exe').exists()
            assert snapshot(ARP) is None,'Installed apps entry was not removed.'
            assert personal.read_text()=='keep my work'
            assert normalized(snapshot(KEYS[0]))==normalized(expected),'Previous terminal preference was not restored.'
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER,r'Environment') as key:
                assert winreg.QueryValueEx(key,'Path')[0]==previous
            print('Native uninstall removed package files, Installed apps entry and PATH registration; restored the previous default and kept personal files.')
    finally:
        for key,data in saved.items(): restore(key,data)
        print('Original account registrations and terminal preferences restored.')

if __name__=='__main__': main()
