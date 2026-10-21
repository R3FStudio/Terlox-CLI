#include "InstalledApplication.h"
#include "integration/defaults/DefaultTerminal.h"
#include "integration/windows/HandoffIds.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <wrl/client.h>
namespace terlox::setup {
static const QString classes="HKEY_CURRENT_USER\\Software\\Classes\\";
static QString normalized(QString path) { return QDir::cleanPath(QDir::fromNativeSeparators(path)).toCaseFolded(); }
static QString program(const QString& command) {
    int count=0; auto** args=CommandLineToArgvW(command.toStdWString().c_str(),&count); QString result;
    if (args) { if (count) result=QString::fromWCharArray(args[0]); LocalFree(args); } return result;
}
static QString value(const QString& path) { return QSettings(path,QSettings::NativeFormat).value(".").toString(); }
static bool removeKey(const QString& path,QString& error) {
    QSettings registry(path,QSettings::NativeFormat); registry.remove(""); registry.sync();
    if (registry.status()!=QSettings::NoError) { error="Cannot remove Windows registration: "+path; return false; } return true;
}
static bool shortcut(const QString& path,const QString& target,QString& error) {
    if (!QFileInfo::exists(path)) return true;
    Microsoft::WRL::ComPtr<IShellLinkW> link; Microsoft::WRL::ComPtr<IPersistFile> file;
    if (FAILED(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&link))) || FAILED(link.As(&file)) || FAILED(file->Load(path.toStdWString().c_str(),STGM_READ))) { error="Cannot inspect shortcut: "+path; return false; }
    wchar_t executable[32768]{};
    if (FAILED(link->GetPath(executable,32768,nullptr,SLGP_RAWPATH))) { error="Cannot inspect shortcut target: "+path; return false; }
    if (normalized(QString::fromWCharArray(executable))!=normalized(target)) return true;
    file.Reset(); link.Reset();
    if (!QFile::remove(path)) { error="Cannot remove shortcut: "+path; return false; } return true;
}
bool removeInstalledIntegration(const QString& directory,QString& error,QStringList& notes) {
    const auto initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    const auto finish=[initialized](bool result) { if (SUCCEEDED(initialized)) CoUninitialize(); return result; };
    const auto executable=directory+"/terlox.exe";
    const auto terminal=classes+"CLSID\\"+TerminalClass;
    if (normalized(program(value(terminal+"\\LocalServer32")))==normalized(executable)) {
        const auto restored=restoreDefaultTerminal();
        if (!restored.success) {
            QSettings startup("HKEY_CURRENT_USER\\Console\\%%Startup",QSettings::NativeFormat);
            if (startup.value("DelegationTerminal").toString().compare(TerminalClass,Qt::CaseInsensitive)==0) {
                startup.remove("DelegationTerminal"); startup.remove("DelegationConsole"); startup.sync();
                if (startup.status()!=QSettings::NoError) { error="Cannot clear the default terminal preference. Select another terminal before uninstalling."; return finish(false); }
                notes << "No earlier default was saved; Windows will choose its terminal provider.";
            }
        } else notes << restored.message;
        if (!removeKey(terminal,error)) return finish(false);
    }
    const auto proxy=classes+"CLSID\\"+ProxyClass;
    const auto proxyPath=normalized(value(proxy+"\\InprocServer32"));
    if (proxyPath.startsWith(normalized(directory)+'/')) {
        for (const auto* id:{"{E686C757-9A35-4A1C-B3CE-0BCC8B5C69F4}","{746E6BC0-AB05-4E38-AB14-71E86763141F}","{59D55CCE-FC8A-48B4-ACE8-0A9286C6557F}","{AA6B364F-4A50-4176-9002-0AE755E7B5EF}","{6F23DA90-15C5-4203-9DB0-64E73F1B1B00}"}) {
            const auto interfaceKey=classes+"Interface\\"+id+"\\ProxyStubClsid32";
            if (value(interfaceKey).compare(ProxyClass,Qt::CaseInsensitive)==0 && !removeKey(interfaceKey,error)) return finish(false);
        }
        if (!removeKey(proxy,error)) return finish(false);
    }
    const auto console=classes+"CLSID\\"+ConsoleClass;
    if (normalized(program(value(console+"\\LocalServer32")))==normalized(directory+"/OpenConsole.exe") && !removeKey(console,error)) return finish(false);
    const auto appPath=QString("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\terlox.exe");
    if (normalized(value(appPath))==normalized(executable) && !removeKey(appPath,error)) return finish(false);
    for (const auto& location:{QString("Directory\\shell\\Terlox"),QString("Directory\\Background\\shell\\Terlox")}) {
        const auto key=classes+location;
        if (normalized(program(value(key+"\\command")))==normalized(executable) && !removeKey(key,error)) return finish(false);
    }
    for (const auto location:{QStandardPaths::ApplicationsLocation,QStandardPaths::DesktopLocation})
        if (!shortcut(QStandardPaths::writableLocation(location)+"/Terlox.lnk",executable,error)) return finish(false);
    QSettings environment("HKEY_CURRENT_USER\\Environment",QSettings::NativeFormat);
    const auto old=environment.value("Path").toString(); auto entries=old.split(';');
    for (auto it=entries.begin();it!=entries.end();) {
        auto path=it->trimmed(); if (path.startsWith('"') && path.endsWith('"')) path=path.mid(1,path.size()-2);
        wchar_t expanded[32768]{}; const auto length=ExpandEnvironmentStringsW(path.toStdWString().c_str(),expanded,32768);
        if (length && length<=32768) path=QString::fromWCharArray(expanded);
        if (normalized(path)==normalized(directory)) it=entries.erase(it); else ++it;
    }
    if (old!=entries.join(';')) {
        environment.setValue("Path",entries.join(';')); environment.sync();
        if (environment.status()!=QSettings::NoError) { error="Cannot remove Terlox from your user PATH."; return finish(false); }
        DWORD_PTR result; SendMessageTimeoutW(HWND_BROADCAST,WM_SETTINGCHANGE,0,reinterpret_cast<LPARAM>(L"Environment"),SMTO_ABORTIFHUNG,1000,&result);
    }
    return finish(true);
}
}
