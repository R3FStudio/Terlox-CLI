#include "UninstallPlan.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#ifdef Q_OS_WIN
#include "installer/windows/InstalledApplication.h"
#endif
namespace terlox::setup {
bool uninstallPackage(const QString& directory,QString& error,QStringList& notes,bool removeIntegration) {
    UninstallPlan plan; if (!buildUninstallPlan(directory,plan,error)) return false;
#ifdef Q_OS_WIN
    if (!checkInstallationNotRunning(plan,error)) return false;
    if (removeIntegration && !removeInstalledIntegration(plan.directory,error,notes)) return false;
#else
    Q_UNUSED(removeIntegration);
#endif
    QStringList failed;
    for (const auto& relative:plan.files) {
        const auto path=plan.directory+'/'+relative;
        if ((QFileInfo::exists(path) || QFileInfo(path).isSymLink()) && !QFile::remove(path)) failed << relative;
    }
    if (!failed.isEmpty()) { error="Some installed files could not be removed. Close Terlox and its sessions, then retry.\n"+failed.join('\n'); return false; }
#ifdef Q_OS_WIN
    if (removeIntegration && !removeUninstallRegistration(plan.directory,error)) return false;
#endif
    if (!QFile::remove(plan.directory+"/.terlox-install.json")) { error="Cannot remove the installation inventory."; return false; }
    for (const auto& folder:plan.directories) QDir().rmdir(plan.directory+'/'+folder);
    if (!QDir().rmdir(plan.directory)) notes << "Files you added to the installation folder were kept.";
    notes << "Personal settings and shell history were kept in your account."; return true;
}
}
