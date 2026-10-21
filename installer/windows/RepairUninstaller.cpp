#include "InstalledApplication.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
namespace terlox::setup {
bool repairUninstaller(const QString& package,const QString& directory,QString& error) {
    UninstallPlan installed; if (!buildUninstallPlan(directory,installed,error)) return false;
    InstallPlan source;
    // Also verifies that existing files belong to the prior installation and
    // that package/installation paths cannot escape through directory links.
    if (!buildInstallPlan({package,directory,false,false,false,true},source,error)) return false;
    auto saveInventory=[&] {
        QSaveFile manifest(installed.directory+"/.terlox-install.json");
        const auto data=QJsonDocument(QJsonObject{{"application","app.terlox.terminal"},{"version",installed.version},{"files",QJsonArray::fromStringList(installed.files)}}).toJson();
        if (!manifest.open(QIODevice::WriteOnly) || manifest.write(data)!=data.size() || !manifest.commit()) { error="Cannot save the repaired installation inventory."; return false; }
        return true;
    };
    for (const auto* name:{"terlox-maintenance.exe","Uninstall.exe"}) {
        QFile input(source.options.source+'/'+name); QSaveFile output(installed.directory+'/'+name);
        if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly)) { error="Cannot add "+QString(name)+". Close any existing uninstaller and retry."; return false; }
        while (!input.atEnd()) {
            const auto bytes=input.read(1024*1024);
            if (bytes.isEmpty() && input.error()!=QFile::NoError) { error=input.errorString(); return false; }
            if (output.write(bytes)!=bytes.size()) { error=output.errorString(); return false; }
        }
        if (!output.commit()) { error=output.errorString(); return false; }
        const bool added=!installed.files.contains(name);
        if (added) installed.files << name;
        // Record each addition before copying the next file, so a retry still
        // recognizes ownership after a partial repair.
        if (!saveInventory()) { if (added) QFile::remove(installed.directory+'/'+name); return false; }
    }
    source.options.destination=installed.directory; source.version=installed.version; source.bytes=0;
    for (const auto& relative:installed.files) source.bytes+=QFileInfo(installed.directory+'/'+relative).size();
    return registerInstalledApplication(source,error);
}
}
