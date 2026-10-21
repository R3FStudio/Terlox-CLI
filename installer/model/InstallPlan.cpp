#include "InstallPlan.h"
#include "integration/defaults/DefaultTerminal.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QSet>
namespace terlox::setup {
static constexpr auto marker=".terlox-package.json";
static constexpr auto manifest=".terlox-install.json";
static QString resolved(QString path) {
    const QFileInfo info(QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
    if (info.exists()) return info.canonicalFilePath();
    const auto parent=info.dir().absolutePath();
    if (parent==info.absoluteFilePath()) return parent;
    return QDir(resolved(parent)).filePath(info.fileName());
}
static bool within(const QString& path,const QString& parent) {
#ifdef Q_OS_WIN
    constexpr auto sensitivity=Qt::CaseInsensitive;
#else
    constexpr auto sensitivity=Qt::CaseSensitive;
#endif
    return path.compare(parent,sensitivity)==0 || path.startsWith(parent+'/',sensitivity);
}
QString defaultDestination() {
#ifdef Q_OS_WIN
    return QDir(qEnvironmentVariable("LOCALAPPDATA",QDir::homePath()+"/AppData/Local")).filePath("Programs/Terlox");
#elif defined(Q_OS_MACOS)
    return QDir::homePath()+"/Applications/Terlox";
#else
    return QDir::homePath()+"/.local/opt/terlox";
#endif
}
bool canSelectDefaultTerminal() {
    return supportsDefaultTerminal();
}
QString defaultTerminalExplanation() {
    return defaultTerminalDetails();
}
bool buildInstallPlan(const InstallOptions& options,InstallPlan& plan,QString& error) {
    error.clear(); plan={}; plan.options=options;
    if (options.source.isEmpty() || options.destination.trimmed().isEmpty()) { error="Choose a package and an installation folder."; return false; }
    plan.options.source=resolved(options.source); plan.options.destination=resolved(options.destination);
    const auto source=plan.options.source,destination=plan.options.destination;
    if (within(source,destination)||within(destination,source)) { error="Install outside the extracted package folder."; return false; }
    if (QDir(destination).isRoot() || destination==QDir::homePath()) { error="Choose a separate Terlox installation folder."; return false; }
    for (const auto& location:{QStandardPaths::DesktopLocation,QStandardPaths::DocumentsLocation,QStandardPaths::ApplicationsLocation}) {
        const auto reserved=QStandardPaths::writableLocation(location);
        if (!reserved.isEmpty() && destination==resolved(reserved)) { error="Choose a Terlox subfolder rather than a shared system folder."; return false; }
    }
    QFile description(QDir(source).filePath(marker));
    if (!description.open(QIODevice::ReadOnly)) { error="This is not a deployed Terlox package. Extract the complete release before starting setup."; return false; }
    const auto meta=QJsonDocument::fromJson(description.readAll()).object();
    if (meta.value("application").toString()!="app.terlox.terminal") { error="The Terlox package marker is invalid."; return false; }
#ifdef Q_OS_WIN
    const QString expectedPlatform="windows";
#elif defined(Q_OS_MACOS)
    const QString expectedPlatform="macos";
#else
    const QString expectedPlatform="linux";
#endif
    if (meta.value("platform").toString()!=expectedPlatform) { error="This package is for a different operating system."; return false; }
    plan.version=meta.value("version").toString(); plan.executable=meta.value("executable").toString();
    if (plan.executable.isEmpty() || QDir::isAbsolutePath(plan.executable) || plan.executable.contains("..") || !QFileInfo::exists(QDir(source).filePath(plan.executable))) { error="The package's terminal executable is missing or invalid."; return false; }
    if (options.defaultTerminal && (!options.desktopIntegration || !canSelectDefaultTerminal())) { error="The selected default-terminal integration is unavailable."; return false; }
#ifdef Q_OS_WIN
    if (options.registerApplication && (!QFileInfo(source+"/Uninstall.exe").isFile() || !QFileInfo(source+"/terlox-maintenance.exe").isFile())) { error="The package is missing its Windows uninstaller. Use the complete current installer."; return false; }
#endif
    QSet<QString> owned;
    QFile previous(QDir(destination).filePath(manifest));
    if (previous.exists()) {
        if (!previous.open(QIODevice::ReadOnly)) { error="Cannot read the existing installation manifest."; return false; }
        const auto old=QJsonDocument::fromJson(previous.readAll()).object();
        if (old.value("application").toString()!="app.terlox.terminal") { error="The existing installation manifest is invalid."; return false; }
        for (const auto& file:old.value("files").toArray()) owned.insert(file.toString());
    }
    QDirIterator entries(source,QDir::AllEntries|QDir::Hidden|QDir::NoDotAndDotDot,QDirIterator::Subdirectories);
    while (entries.hasNext()) {
        entries.next(); const auto info=entries.fileInfo();
        if (info.isDir() && !info.isSymLink()) continue;
        const auto relative=QDir(source).relativeFilePath(info.absoluteFilePath());
        if (relative==manifest) continue;
        if (info.isSymLink() && !within(info.canonicalFilePath(),source)) { error="The package contains a link outside its folder: "+relative; return false; }
        const auto target=QDir(destination).filePath(relative);
        if (!within(resolved(target),destination)) { error="An installation path points outside the chosen folder: "+relative; return false; }
        if (QFileInfo::exists(target) && !owned.contains(relative)) { error="Setup will not overwrite an unrelated file: "+target+". Choose an empty folder."; return false; }
        plan.files << relative; if (!info.isSymLink()) plan.bytes+=info.size();
    }
    if (plan.files.isEmpty()) { error="The package is empty."; return false; }
    plan.files.sort(); return true;
}
}
