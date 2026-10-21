#include "integration/defaults/DefaultTerminal.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QRegularExpression>
namespace terlox {
static const QString entry="app.terlox.terminal.desktop";
static QString configRoot() { return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation); }
static QString savedPath() { return configRoot()+"/terlox/default-terminal.json"; }
static QByteArray read(const QString& path) { QFile file(path); return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray(); }
static bool save(const QString& path,const QByteArray& data) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
    QSaveFile file(path); return file.open(QIODevice::WriteOnly) && file.write(data)==data.size() && file.commit();
}
static QStringList preferenceFiles() {
    QStringList result{configRoot()+"/xdg-terminals.list"};
    for (const auto& desktop:qEnvironmentVariable("XDG_CURRENT_DESKTOP").split(':',Qt::SkipEmptyParts)) {
        bool safe=true; for (const auto c:desktop) if (!c.isLetterOrNumber() && c!='-' && c!='_') safe=false;
        if (safe) result << configRoot()+'/'+desktop.toLower()+"-xdg-terminals.list";
    }
    result.removeDuplicates(); return result;
}
static bool selected(const QString& path) {
    for (const auto& line:QString::fromUtf8(read(path)).split('\n')) {
        const auto value=line.trimmed(); if (value.isEmpty() || value.startsWith('#')) continue;
        return value==entry;
    }
    return false;
}
bool supportsDefaultTerminal() { return true; }
QString defaultTerminalDetails() { return "Select Terlox for xdg-terminal-exec in this account. Desktop environments and applications with their own terminal setting may need a separate selection. Command: terloxctl default-terminal set"; }
DefaultTerminalResult selectDefaultTerminal(const QString& executable) {
    auto path=QFileInfo(executable).absoluteFilePath();
    const auto appdir=QFileInfo(qEnvironmentVariable("APPDIR")).canonicalFilePath();
    const auto image=qEnvironmentVariable("APPIMAGE");
    if (!appdir.isEmpty() && !image.isEmpty() && QFileInfo(path).canonicalFilePath().startsWith(appdir+'/')) path=image;
    if (!QFileInfo(path).isExecutable()) return {false,"Install Terlox before selecting it as the default terminal."};
    auto quote=[](QString value) { value.replace('\\',"\\\\"); value.replace('"',"\\\""); value.replace('`',"\\`"); value.replace('$',"\\$"); value.replace('%',"%%"); return '"'+value+'"'; };
    const auto launcher=QString("[Desktop Entry]\nType=Application\nName=Terlox\nExec=%1\nIcon=app.terlox.terminal\nTerminal=false\nCategories=System;TerminalEmulator;\nX-TerminalArgExec=-e\nX-TerminalArgDir=--working-directory\n").arg(quote(path));
    const auto desktop=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/applications/"+entry;
    if (!save(desktop,launcher.toUtf8())) return {false,"Could not register the per-user Terlox launcher."};
    auto previous=QJsonDocument::fromJson(read(savedPath())).object();
    for (const auto& file:preferenceFiles()) if (!previous.contains(file) && !selected(file)) {
        previous[file]=QJsonObject{{"present",QFileInfo::exists(file)},{"data",QString::fromLatin1(read(file).toBase64())}};
    }
    if (!save(savedPath(),QJsonDocument(previous).toJson())) return {false,"Could not save the previous terminal preference."};
    for (const auto& file:preferenceFiles()) {
        auto lines=QString::fromUtf8(read(file)).split('\n');
        for (auto it=lines.begin();it!=lines.end();) if (it->trimmed()==entry) it=lines.erase(it); else ++it;
        while (!lines.isEmpty() && lines.last().isEmpty()) lines.removeLast();
        if (!save(file,(entry+'\n'+lines.join('\n')+(lines.isEmpty()?"":"\n")).toUtf8())) return {false,"Could not save "+file};
    }
    const bool launcherInstalled=!QStandardPaths::findExecutable("xdg-terminal-exec").isEmpty();
    return {true,launcherInstalled?"Terlox is selected for xdg-terminal-exec. Restore with: terloxctl default-terminal restore":"Terlox preference saved. Install xdg-terminal-exec for applications that use it. Other desktop terminal settings are separate."};
}
DefaultTerminalResult defaultTerminalStatus() {
    bool active=true; for (const auto& path:preferenceFiles()) active=active && selected(path);
    return {true,active?"Terlox is selected for xdg-terminal-exec.":"Terlox is not selected for xdg-terminal-exec. Run: terloxctl default-terminal set"};
}
DefaultTerminalResult restoreDefaultTerminal() {
    const auto previous=QJsonDocument::fromJson(read(savedPath())).object();
    if (previous.isEmpty()) return {false,"No previous terminal preference was saved."};
    for (auto it=previous.begin();it!=previous.end();++it) {
        const QFileInfo file(it.key());
        const QRegularExpression allowed("^(?:[a-zA-Z0-9_-]+-)?xdg-terminals\\.list$");
        if (file.absolutePath()!=QDir(configRoot()).absolutePath() || !allowed.match(file.fileName()).hasMatch() || !selected(it.key())) continue;
        const auto value=it.value().toObject();
        if (value.value("present").toBool()) { if (!save(it.key(),QByteArray::fromBase64(value.value("data").toString().toLatin1()))) return {false,"Could not restore "+it.key()}; }
        else if (!QFile::remove(it.key())) return {false,"Could not remove "+it.key()};
    }
    QFile::remove(savedPath()); return {true,"Restored previous xdg-terminal-exec preferences; later selections were retained."};
}
}
