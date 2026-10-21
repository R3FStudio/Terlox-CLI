#include "integration/defaults/DefaultTerminal.h"
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QSettings>
#include <CoreServices/CoreServices.h>
namespace terlox {
static CFStringRef bundle=CFSTR("app.terlox.terminal");
static CFStringRef scriptType=CFSTR("com.apple.terminal.shell-script");
static QString currentHandler() {
    const auto handler=LSCopyDefaultRoleHandlerForContentType(scriptType,kLSRolesShell);
    if (!handler) return {};
    const auto size=CFStringGetLength(handler); QString result(size,Qt::Uninitialized);
    CFStringGetCharacters(handler,CFRangeMake(0,size),reinterpret_cast<UniChar*>(result.data())); CFRelease(handler); return result;
}
bool supportsDefaultTerminal() { return true; }
QString defaultTerminalDetails() { return "Use Terlox to open terminal .command files through Launch Services. macOS has no universal replacement for every terminal launcher. Command: terloxctl default-terminal set"; }
DefaultTerminalResult selectDefaultTerminal(const QString& executable) {
    const auto app=QFileInfo(QFileInfo(executable).absolutePath()+"/../..").canonicalFilePath();
    if (!app.endsWith(".app") || !QFileInfo::exists(app+"/Contents/Info.plist")) return {false,"Run this command from the installed Terlox.app bundle."};
    const auto bytes=QFile::encodeName(app);
    const auto url=CFURLCreateFromFileSystemRepresentation(nullptr,reinterpret_cast<const UInt8*>(bytes.constData()),bytes.size(),true);
    const auto registered=LSRegisterURL(url,true); CFRelease(url);
    if (registered!=noErr) return {false,QString("Launch Services registration failed (%1).").arg(registered)};
    QSettings saved("Terlox","DefaultTerminal"); const auto old=currentHandler();
    if (old.compare("app.terlox.terminal",Qt::CaseInsensitive)!=0) { saved.setValue("previous",old); saved.sync(); if (saved.status()!=QSettings::NoError) return {false,"Could not save the previous terminal handler."}; }
    const auto result=LSSetDefaultRoleHandlerForContentType(scriptType,kLSRolesShell,bundle);
    return {result==noErr,result==noErr?"Terlox now opens terminal .command files. Editors and Finder terminal services can have separate settings.":QString("macOS did not accept the terminal handler (%1).").arg(result)};
}
DefaultTerminalResult defaultTerminalStatus() { return {true,currentHandler().compare("app.terlox.terminal",Qt::CaseInsensitive)==0?"Terlox is the Launch Services terminal .command handler.":"Terlox is not the terminal .command handler. Run: terloxctl default-terminal set"}; }
DefaultTerminalResult restoreDefaultTerminal() {
    if (currentHandler().compare("app.terlox.terminal",Qt::CaseInsensitive)!=0) return {true,"Another terminal handler is selected; leaving it unchanged."};
    QSettings saved("Terlox","DefaultTerminal"); const auto previous=saved.value("previous").toString();
    if (previous.isEmpty()) return {false,"No previous handler was saved. Select another app in Finder's Open With settings."};
    const auto value=CFStringCreateWithCharacters(nullptr,reinterpret_cast<const UniChar*>(previous.utf16()),previous.size());
    const auto result=LSSetDefaultRoleHandlerForContentType(scriptType,kLSRolesShell,value); CFRelease(value);
    return {result==noErr,result==noErr?"Restored the previous terminal .command handler.":"macOS could not restore the previous handler."};
}
}
