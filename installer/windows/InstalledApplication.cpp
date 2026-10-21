#include "InstalledApplication.h"
#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
namespace terlox::setup {
bool registerInstalledApplication(const InstallPlan& plan,QString& error) {
    const auto root=QDir::toNativeSeparators(plan.options.destination);
    if (!QFileInfo(plan.options.destination+"/Uninstall.exe").isFile()) { error="The package is missing Uninstall.exe."; return false; }
    QSettings registry(UninstallRegistryKey,QSettings::NativeFormat);
    registry.setValue("DisplayName","Terlox"); registry.setValue("DisplayVersion",plan.version);
    registry.setValue("Publisher","R3F Studio"); registry.setValue("InstallLocation",root);
    registry.setValue("DisplayIcon",root+"\\terlox.exe,0");
    registry.setValue("UninstallString",'"'+root+"\\Uninstall.exe\"");
    registry.setValue("QuietUninstallString",'"'+root+"\\Uninstall.exe\" /S");
    registry.setValue("InstallDate",QDate::currentDate().toString("yyyyMMdd"));
    registry.setValue("EstimatedSize",quint32(std::min<qint64>((plan.bytes+1023)/1024,0xffffffff)));
    registry.setValue("NoModify",1); registry.setValue("NoRepair",1); registry.sync();
    if (registry.status()!=QSettings::NoError) { error="Windows could not register Terlox in Installed apps."; return false; }
    return true;
}
bool removeUninstallRegistration(const QString& directory,QString& error) {
    QSettings registry(UninstallRegistryKey,QSettings::NativeFormat);
    if (QDir::cleanPath(QDir::fromNativeSeparators(registry.value("InstallLocation").toString())).compare(QDir::cleanPath(directory),Qt::CaseInsensitive)!=0) return true;
    registry.remove(""); registry.sync();
    if (registry.status()!=QSettings::NoError) { error="Windows could not remove the Installed apps entry."; return false; }
    return true;
}
}
