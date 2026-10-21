#include "UninstallPlan.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <algorithm>
namespace terlox::setup {
static bool link(const QFileInfo& file) {
#ifdef Q_OS_WIN
    return file.isSymLink() || file.isJunction();
#else
    return file.isSymLink();
#endif
}
bool buildUninstallPlan(const QString& directory,UninstallPlan& plan,QString& error) {
    error.clear(); plan={}; const QFileInfo root(directory);
    if (!root.isDir() || link(root)) { error="Choose the actual Terlox installation folder, not a link."; return false; }
    plan.directory=root.canonicalFilePath();
    if (QDir(plan.directory).isRoot() || plan.directory==QDir::homePath()) { error="Refusing to uninstall from a shared root folder."; return false; }
    const auto manifest=plan.directory+"/.terlox-install.json";
    if (link(QFileInfo(manifest))) { error="The installation inventory must not be a link."; return false; }
    QFile inventory(manifest);
    if (!inventory.open(QIODevice::ReadOnly)) { error="Cannot read the Terlox installation inventory. Choose a valid installed copy; no files were removed."; return false; }
    if (inventory.size()>16*1024*1024) { error="The installation inventory is too large."; return false; }
    const auto document=QJsonDocument::fromJson(inventory.readAll()).object();
    if (document.value("application").toString()!="app.terlox.terminal" || !document.value("files").isArray()) { error="This is not a valid Terlox installation inventory."; return false; }
    plan.version=document.value("version").toString(); QSet<QString> directories,files;
    for (const auto& value:document.value("files").toArray()) {
        const auto file=value.toString(); const auto normalized=QDir::fromNativeSeparators(file);
        if (!value.isString() || file.isEmpty() || QDir::isAbsolutePath(normalized) || normalized.contains(':') || normalized.contains('\0') || normalized.split('/').contains("..") || QDir::cleanPath(normalized)!=normalized || normalized==".") { error="Unsafe path in installation inventory: "+file; return false; }
#ifdef Q_OS_WIN
        for (const auto& part:normalized.split('/')) if (part.endsWith(' ') || part.endsWith('.')) { error="Ambiguous Windows path in installation inventory: "+file; return false; }
#endif
        auto parent=QFileInfo(normalized).path();
        while (parent!=".") {
            const QFileInfo info(plan.directory+'/'+parent);
            if (link(info) || (info.exists() && !info.isDir())) { error="An installation folder was replaced with a link or file: "+parent; return false; }
            directories.insert(parent); parent=QFileInfo(parent).path();
        }
        const QFileInfo target(plan.directory+'/'+normalized);
        if (target.isDir() && !link(target)) { error="An installed file was replaced by a directory: "+normalized; return false; }
        if (!files.contains(normalized)) { files.insert(normalized); plan.files << normalized; }
    }
    if (plan.files.isEmpty()) { error="The installation inventory contains no files."; return false; }
    plan.directories=directories.values();
    std::sort(plan.directories.begin(),plan.directories.end(),[](const auto& a,const auto& b) { return a.size()>b.size(); });
    return true;
}
}
