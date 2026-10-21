#include "InstallEngine.h"
#include "installer/integration/DesktopIntegration.h"
#include "installer/uninstall/UninstallPlan.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QMap>
#ifdef Q_OS_WIN
#include "installer/windows/InstalledApplication.h"
#endif
namespace terlox::setup {
static bool atomicCopy(const QString& source,const QString& destination,QString& error) {
    QFile input(source); QSaveFile output(destination);
    if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly)) { error="Cannot open "+destination+": "+(input.isOpen()?output.errorString():input.errorString()); return false; }
    while (!input.atEnd()) {
        const auto bytes=input.read(1024*1024);
        if (bytes.isEmpty() && input.error()!=QFileDevice::NoError) { error=input.errorString(); return false; }
        if (output.write(bytes)!=bytes.size()) { error=output.errorString(); return false; }
    }
    output.setPermissions(input.permissions());
    if (!output.commit()) { error="Cannot replace "+destination+": "+output.errorString()+". Close all installed Terlox processes and retry."; return false; }
    return true;
}
InstallResult installPackage(const InstallPlan& requested,const Progress& progress) {
    InstallResult result; InstallPlan plan;
    if (!buildInstallPlan(requested.options,plan,result.message)) return result;
    QStringList inventory=plan.files;
    if (QFileInfo::exists(plan.options.destination+"/.terlox-install.json")) {
        UninstallPlan previous;
        if (!buildUninstallPlan(plan.options.destination,previous,result.message)) return result;
        // Older versioned runtime files still belong to Terlox even when an
        // upgrade stops shipping them. Keep ownership until they are removed.
        for (const auto& file:previous.files) if (!inventory.contains(file)) inventory << file;
    }
    QTemporaryDir backup; if (!backup.isValid()) { result.message="Cannot create an installation rollback folder."; return result; }
    QStringList written,existing;
    QMap<QString,QString> oldLinks;
    auto rollback=[&] {
        for (auto it=written.crbegin();it!=written.crend();++it) {
            const auto target=QDir(plan.options.destination).filePath(*it);
            if (!QFile::remove(target)) result.notes << "Could not roll back: "+target;
            if (existing.contains(*it)) {
                QString error;
                if (oldLinks.contains(*it)) {
                    if (!QFile::link(oldLinks.value(*it),target)) result.notes << "Cannot restore bundle link: "+target;
                } else if (!atomicCopy(QDir(backup.path()).filePath(*it),target,error)) result.notes << error;
            }
        }
    };
    for (int i=0;i<plan.files.size();++i) {
        const auto relative=plan.files[i],source=QDir(plan.options.source).filePath(relative),target=QDir(plan.options.destination).filePath(relative);
        if (progress) progress(i*90/plan.files.size(),relative);
        if (!QDir().mkpath(QFileInfo(target).absolutePath())) { result.message="Cannot create the installation folder."; rollback(); return result; }
        if (QFileInfo(target).isSymLink()) {
            oldLinks.insert(relative,QFileInfo(target).symLinkTarget()); existing << relative;
        } else if (QFileInfo::exists(target)) {
            const auto saved=QDir(backup.path()).filePath(relative); QDir().mkpath(QFileInfo(saved).absolutePath());
            if (!atomicCopy(target,saved,result.message)) { rollback(); return result; }
            existing << relative;
        }
        const QFileInfo info(source);
        bool ok=false;
        if (info.isSymLink()) {
            // Qt framework links must stay relative after the package is moved.
            if (QFileInfo::exists(target) || QFileInfo(target).isSymLink()) {
                if (!QFile::remove(target)) { result.message="Cannot replace bundle link: "+relative; rollback(); return result; }
                written << relative;
            }
            const auto link=QDir(QFileInfo(source).absolutePath()).relativeFilePath(info.symLinkTarget());
            ok=QFile::link(link,target); if (!ok) result.message="Cannot create bundle link: "+relative;
        } else ok=atomicCopy(source,target,result.message);
        if (!ok) { rollback(); return result; }
        if (!written.contains(relative)) written << relative;
    }
    const auto manifestPath=QDir(plan.options.destination).filePath(".terlox-install.json");
    QSaveFile manifest(manifestPath);
    const auto data=QJsonDocument(QJsonObject{{"application","app.terlox.terminal"},{"version",plan.version},{"files",QJsonArray::fromStringList(inventory)}}).toJson();
    if (!manifest.open(QIODevice::WriteOnly) || manifest.write(data)!=data.size() || !manifest.commit()) { result.message="Cannot save the installation manifest."; rollback(); return result; }
    if (progress) progress(95,"Desktop integration");
#ifdef Q_OS_WIN
    if (plan.options.registerApplication && !registerInstalledApplication(plan,result.message)) {
        result.message="Application files were installed, but uninstall registration failed: "+result.message; return result;
    }
    if (plan.options.registerApplication) result.notes << "Uninstall Terlox through Windows Installed apps or Uninstall.exe in its installation folder.";
#endif
    if (plan.options.desktopIntegration && !registerDesktop(plan,result.message,result.notes)) {
        result.message="The application files were installed, but desktop integration failed: "+result.message+". You can retry setup."; return result;
    }
    result.success=true; result.message="Terlox "+plan.version+" is installed.";
    if (progress) progress(100,result.message); return result;
}
}
