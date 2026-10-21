#include "integration/defaults/DefaultTerminal.h"
#include "HandoffIds.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QProcess>
#include <windows.h>
#include <objbase.h>
namespace terlox {
static const QString startup="HKEY_CURRENT_USER\\Console\\%%Startup";
static const QString backup="HKEY_CURRENT_USER\\Software\\Terlox\\DefaultTerminal";
static const QString classes="HKEY_CURRENT_USER\\Software\\Classes\\";
static const QStringList values{"DelegationConsole","DelegationTerminal"};
static bool isTerlox(QSettings& registry) { return registry.value("DelegationTerminal").toString().compare(TerminalClass,Qt::CaseInsensitive)==0 && registry.value("DelegationConsole").toString().compare(ConsoleClass,Qt::CaseInsensitive)==0; }
bool supportsDefaultTerminal() {
    using VersionFunction=LONG(WINAPI*)(OSVERSIONINFOW*);
    auto function=reinterpret_cast<VersionFunction>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlGetVersion"));
    OSVERSIONINFOW version{}; version.dwOSVersionInfoSize=sizeof(version);
    if (!function || function(&version)!=0 || version.dwMajorVersion<10) return false;
    if (version.dwBuildNumber>=22000) return true;
    if (version.dwBuildNumber!=19045) return false;
    QSettings current("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",QSettings::NativeFormat);
    return current.value("UBR").toUInt()>=3031;
}
QString defaultTerminalDetails() { return supportsDefaultTerminal()?"Receive newly launched console applications in Terlox for this account. You can also run: terloxctl default-terminal set":"Windows default-terminal handoff requires Windows 11, or Windows 10 22H2 build 19045.3031 or newer. Shortcuts and explicit Terlox sessions still work."; }
static bool writeRegistry(const QString& path,const QString& key,const QString& value) {
    QSettings registry(path,QSettings::NativeFormat); registry.setValue(key,value); registry.sync(); return registry.status()==QSettings::NoError;
}
static bool registerProxy(const QString& directory) {
    const auto path=QDir::toNativeSeparators(directory+"/" TERLOX_HANDOFF_PROXY_FILE);
    if (!writeRegistry(classes+"CLSID\\"+ProxyClass+"\\InprocServer32",".",path) || !writeRegistry(classes+"CLSID\\"+ProxyClass+"\\InprocServer32","ThreadingModel","Both")) return false;
    // Keep an existing system/packaged proxy. Only supply missing interfaces.
    for (const auto* value:{"{E686C757-9A35-4A1C-B3CE-0BCC8B5C69F4}","{746E6BC0-AB05-4E38-AB14-71E86763141F}","{59D55CCE-FC8A-48B4-ACE8-0A9286C6557F}","{AA6B364F-4A50-4176-9002-0AE755E7B5EF}","{6F23DA90-15C5-4203-9DB0-64E73F1B1B00}"}) {
        IID iid; CLSID proxy; IIDFromString(QString(value).toStdWString().c_str(),&iid);
        if (FAILED(CoGetPSClsid(iid,&proxy)) && !writeRegistry(classes+"Interface\\"+value+"\\ProxyStubClsid32",".",ProxyClass)) return false;
    }
    return true;
}
static bool hasConsoleHost() {
    // Packaged COM is not necessarily exposed through HKCR\CLSID. Query the
    // installed package without starting a console host or touching its keys.
    QSettings registry("HKEY_CLASSES_ROOT\\CLSID\\"+QString(ConsoleClass)+"\\LocalServer32",QSettings::NativeFormat);
    if (!registry.value(".").toString().isEmpty()) return true;
    QProcess lookup;
    lookup.start(qEnvironmentVariable("SystemRoot","C:/Windows")+"/System32/WindowsPowerShell/v1.0/powershell.exe",{"-NoLogo","-NoProfile","-NonInteractive","-Command","$p = Get-AppxPackage -Name Microsoft.WindowsTerminal; if ($p) { [Console]::Write($p.InstallLocation) }"});
    return lookup.waitForFinished(20000) && lookup.exitCode()==0 && QFileInfo::exists(QString::fromUtf8(lookup.readAllStandardOutput()).trimmed()+"/OpenConsole.exe");
}
DefaultTerminalResult selectDefaultTerminal(const QString& executable) {
    if (!supportsDefaultTerminal()) return {false,defaultTerminalDetails()};
    const auto file=QFileInfo(executable); const auto directory=file.absolutePath();
    for (const auto& name:{file.fileName(),QString("conpty.dll"),QString("OpenConsole.exe"),QString(TERLOX_HANDOFF_PROXY_FILE),QString("Qt6Core.dll"),QString("Qt6Gui.dll"),QString("Qt6Widgets.dll"),QString("platforms/qwindows.dll")})
        if (!QFileInfo(directory+'/'+name).isFile()) return {false,"Missing installed runtime file: "+directory+'/'+name+". Run the complete Terlox installer. COM launches cannot use a developer shell's Qt SDK path."};
    const auto initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    const auto finish=[initialized](DefaultTerminalResult result) { if (SUCCEEDED(initialized)) CoUninitialize(); return result; };
    if (!registerProxy(directory) || !writeRegistry(classes+"CLSID\\"+TerminalClass+"\\LocalServer32",".",'"'+QDir::toNativeSeparators(file.absoluteFilePath())+"\" -Embedding")) return finish({false,"Could not register the per-user Windows handoff server."});
    // Microsoft's signed host has a fixed release CLSID. Reuse an installed
    // Windows Terminal host; never replace its registration. Supply the bundled
    // signed host only on machines without that provider.
    if (!hasConsoleHost() && !writeRegistry(classes+"CLSID\\"+ConsoleClass+"\\LocalServer32",".",'"'+QDir::toNativeSeparators(directory+"/OpenConsole.exe")+"\" -Embedding")) return finish({false,"Could not register the Microsoft console host."});
    QSettings registry(startup,QSettings::NativeFormat),saved(backup,QSettings::NativeFormat);
    if (!isTerlox(registry)) {
        QJsonObject previous;
        for (const auto& name:values) { previous[name+"Present"]=registry.contains(name); previous[name]=registry.value(name).toString(); }
        saved.setValue("previous",QString::fromUtf8(QJsonDocument(previous).toJson(QJsonDocument::Compact))); saved.sync();
        if (saved.status()!=QSettings::NoError) return finish({false,"Could not save the previous default terminal. Preferences were left unchanged."});
    }
    registry.setValue("DelegationConsole",ConsoleClass); registry.setValue("DelegationTerminal",TerminalClass); registry.sync();
    if (registry.status()!=QSettings::NoError) return finish({false,"Windows could not save the default-terminal preference."});
    return finish({true,"Terlox is now the default terminal for newly launched console applications in this Windows account. Existing sessions stay open. Restore with: terloxctl default-terminal restore"});
}
DefaultTerminalResult defaultTerminalStatus() {
    QSettings registry(startup,QSettings::NativeFormat);
    if (isTerlox(registry)) return {true,"Terlox is the Windows default terminal for this account."};
    return {true,"Terlox is not the Windows default terminal. Select it with: terloxctl default-terminal set"};
}
DefaultTerminalResult restoreDefaultTerminal() {
    QSettings registry(startup,QSettings::NativeFormat),saved(backup,QSettings::NativeFormat);
    if (!isTerlox(registry)) return {true,"The default has changed since Terlox was selected; leaving it unchanged."};
    const auto previous=QJsonDocument::fromJson(saved.value("previous").toString().toUtf8()).object();
    if (previous.isEmpty()) return {false,"No previous default was saved. Choose another terminal in Windows Settings."};
    for (const auto& name:values) if (previous.value(name+"Present").toBool()) registry.setValue(name,previous.value(name).toString()); else registry.remove(name);
    registry.sync();
    const bool success=registry.status()==QSettings::NoError;
    return {success,success?"Restored the previous Windows terminal preference.":"Windows could not restore the previous terminal preference."};
}
}
