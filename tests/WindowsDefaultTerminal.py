"""Opt-in desktop smoke test. Restore every changed registration in finally.

Run on an interactive Windows desktop; this briefly selects the test build as
the default, starts a real new console, and checks for its Terlox window.
"""
import argparse
import ctypes
from ctypes import wintypes
import os
from pathlib import Path
import subprocess
import time
import winreg
import tempfile

TERMINAL='{BD3146DB-28E9-4FF9-AF28-165CA5C99A35}'
PROXY='{7E292492-DA77-4AF8-935D-01AF34B556EB}'
CONSOLE='{2EACA947-7F5F-4CFA-BA87-8F7FBEEFBE69}'
INTERFACES=['E686C757-9A35-4A1C-B3CE-0BCC8B5C69F4','746E6BC0-AB05-4E38-AB14-71E86763141F','59D55CCE-FC8A-48B4-ACE8-0A9286C6557F','AA6B364F-4A50-4176-9002-0AE755E7B5EF','6F23DA90-15C5-4203-9DB0-64E73F1B1B00']
KEYS=[r'Console\%%Startup',r'Software\Terlox\DefaultTerminal']
KEYS += ['Software\\Classes\\CLSID\\'+value for value in [TERMINAL,PROXY,CONSOLE]]
KEYS += [r'Software\Classes\Interface\{'+value+'}' for value in INTERFACES]

def snapshot(path):
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER,path) as key:
            children,values,_=winreg.QueryInfoKey(key)
            return ([winreg.EnumValue(key,i) for i in range(values)],{name:snapshot(path+'\\'+name) for name in [winreg.EnumKey(key,i) for i in range(children)]})
    except FileNotFoundError:
        return None

def delete(path):
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER,path) as key:
            children=[winreg.EnumKey(key,i) for i in range(winreg.QueryInfoKey(key)[0])]
        for child in children: delete(path+'\\'+child)
        winreg.DeleteKey(winreg.HKEY_CURRENT_USER,path)
    except FileNotFoundError:
        pass

def restore(path,data):
    delete(path)
    if data is None: return
    with winreg.CreateKey(winreg.HKEY_CURRENT_USER,path) as key:
        for name,value,kind in data[0]: winreg.SetValueEx(key,name,0,kind,value)
    for child,content in data[1].items(): restore(path+'\\'+child,content)

def normalized(data):
    # Registry value enumeration order is not significant. An empty %%Startup
    # key and an absent key both mean that no terminal preference is stored.
    if data is None: return ([],{})
    return (sorted(data[0]),{name:normalized(value) for name,value in data[1].items()})

user32=ctypes.WinDLL('user32',use_last_error=True)
kernel32=ctypes.WinDLL('kernel32',use_last_error=True)
kernel32.OpenProcess.argtypes=[wintypes.DWORD,wintypes.BOOL,wintypes.DWORD]; kernel32.OpenProcess.restype=wintypes.HANDLE
kernel32.QueryFullProcessImageNameW.argtypes=[wintypes.HANDLE,wintypes.DWORD,wintypes.LPWSTR,ctypes.POINTER(wintypes.DWORD)]
kernel32.CloseHandle.argtypes=[wintypes.HANDLE]
user32.GetWindowThreadProcessId.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.DWORD)]
user32.IsWindowVisible.argtypes=[wintypes.HWND]
user32.PostMessageW.argtypes=[wintypes.HWND,wintypes.UINT,wintypes.WPARAM,wintypes.LPARAM]
ENUM=ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
user32.EnumWindows.argtypes=[ENUM,wintypes.LPARAM]
class GUIInfo(ctypes.Structure):
    _fields_=[('cbSize',wintypes.DWORD),('flags',wintypes.DWORD),('active',wintypes.HWND),('focus',wintypes.HWND),('capture',wintypes.HWND),('menu',wintypes.HWND),('move',wintypes.HWND),('caret',wintypes.HWND),('caretRect',wintypes.RECT)]
user32.GetGUIThreadInfo.argtypes=[wintypes.DWORD,ctypes.POINTER(GUIInfo)]
user32.GetWindowLongPtrW.argtypes=[wintypes.HWND,ctypes.c_int]
user32.GetWindowLongPtrW.restype=ctypes.c_ssize_t
user32.GetForegroundWindow.restype=wintypes.HWND

def windows(executable):
    found={}
    @ENUM
    def each(window,_):
        if not user32.IsWindowVisible(window): return True
        pid=wintypes.DWORD(); user32.GetWindowThreadProcessId(window,ctypes.byref(pid))
        process=kernel32.OpenProcess(0x1000,False,pid.value)
        if process:
            try:
                name=ctypes.create_unicode_buffer(32768); size=wintypes.DWORD(len(name))
                if kernel32.QueryFullProcessImageNameW(process,0,name,ctypes.byref(size)) and Path(name.value)==executable: found[window]=pid.value
            finally: kernel32.CloseHandle(process)
        return True
    user32.EnumWindows(each,0)
    return found

def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--directory',type=Path,required=True)
    args=parser.parse_args(); directory=args.directory.resolve(); executable=directory/'terlox.exe'
    environment=dict(os.environ)
    if directory.name=='bin': environment['PATH']=str(Path('.tools/qt/6.8.3/msvc2022_64/bin').resolve())+os.pathsep+environment['PATH']
    state={key:snapshot(key) for key in KEYS}; baseline=windows(executable); child=None; opened={}
    try:
        # If the installed Terlox is already selected, "restore" correctly means
        # its saved predecessor, not Terlox again. Give this test a separate
        # baseline; finally still restores the user's existing Terlox selection.
        if state[KEYS[0]] and any(name=='DelegationTerminal' and str(value).casefold()==TERMINAL.casefold() for name,value,_ in state[KEYS[0]][0]):
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER,KEYS[0],0,winreg.KEY_SET_VALUE) as key:
                for name in ['DelegationTerminal','DelegationConsole']:
                    try: winreg.DeleteValue(key,name)
                    except FileNotFoundError: pass
        expected=snapshot(KEYS[0])
        result=subprocess.run([str(directory/'terloxctl.exe'),'default-terminal','set'],env=environment,capture_output=True,text=True,timeout=35)
        assert result.returncode==0,(result.stdout,result.stderr)
        print(result.stdout.strip())
        child=subprocess.Popen([str(Path(os.environ['SystemRoot'])/'System32/cmd.exe'),'/d','/k','echo TERLOX_NATIVE_HANDOFF'],creationflags=subprocess.CREATE_NEW_CONSOLE,env=environment)
        deadline=time.monotonic()+20
        while time.monotonic()<deadline:
            opened={window:pid for window,pid in windows(executable).items() if window not in baseline}
            if opened: break
            time.sleep(0.1)
        if not opened:
            diagnostic=subprocess.run(['powershell','-NoProfile','-Command',"Get-CimInstance Win32_Process -Filter \"Name = 'terlox.exe' OR Name = 'OpenConsole.exe' OR Name = 'cmd.exe'\" | Select-Object ProcessId,ParentProcessId,CommandLine | Format-List"],capture_output=True,text=True,timeout=15)
            print(diagnostic.stdout)
        assert opened,'New Windows console did not open a visible Terlox window.'
        print('Native CREATE_NEW_CONSOLE launch opened a Terlox handoff window:',opened)
        assert all(not user32.GetWindowLongPtrW(window,-20)&8 for window in opened),'The handed-off window was permanently topmost.'
        deadline=time.monotonic()+3
        while time.monotonic()<deadline and user32.GetForegroundWindow() not in opened: time.sleep(0.05)
        assert user32.GetForegroundWindow() in opened,'The handed-off terminal did not come to the foreground on launch.'
        time.sleep(0.4)
        with tempfile.TemporaryDirectory(prefix='terlox-native-input-') as temporary:
            proof=Path(temporary)/'typed command.txt'
            window=next(iter(opened)); pid=wintypes.DWORD(); thread=user32.GetWindowThreadProcessId(window,ctypes.byref(pid))
            info=GUIInfo(); info.cbSize=ctypes.sizeof(info)
            assert user32.GetGUIThreadInfo(thread,ctypes.byref(info))
            target=info.focus or window
            for character in f'echo native-input-ok > "{proof}"': user32.PostMessageW(target,0x102,ord(character),1)
            user32.PostMessageW(target,0x100,13,0x001c0001)
            user32.PostMessageW(target,0x101,13,0xc01c0001)
            deadline=time.monotonic()+5
            while time.monotonic()<deadline and not proof.exists(): time.sleep(0.05)
            assert proof.exists(),'Input did not reach the adopted CMD session.'
            assert proof.read_text().strip()=='native-input-ok'
            print('Typed command executed through the adopted ConPTY input stream.')
        for window in opened: user32.PostMessageW(window,0x10,0,0)
        child.wait(timeout=15)
        print('Closing Terlox closed its adopted console session.')
        restored=subprocess.run([str(directory/'terloxctl.exe'),'default-terminal','restore'],env=environment,capture_output=True,text=True,timeout=10)
        assert restored.returncode==0,(restored.stdout,restored.stderr)
        actual=snapshot(KEYS[0])
        assert normalized(actual)==normalized(expected),('Restore command did not preserve the previous preference.',expected,actual)
    finally:
        for window in windows(executable):
            if window not in baseline: user32.PostMessageW(window,0x10,0,0)
        if child and child.poll() is None: child.kill(); child.wait(timeout=10)
        for key,data in state.items(): restore(key,data)
        print('Original terminal preferences and registrations restored.')

if __name__=='__main__': main()
