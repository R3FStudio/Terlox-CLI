#pragma once
#include "installer/model/InstallPlan.h"
#include "installer/uninstall/UninstallPlan.h"
namespace terlox::setup {
inline constexpr auto UninstallRegistryKey="HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Terlox";
bool registerInstalledApplication(const InstallPlan& plan,QString& error);
bool removeUninstallRegistration(const QString& directory,QString& error);
bool removeInstalledIntegration(const QString& directory,QString& error,QStringList& notes);
bool checkInstallationNotRunning(const UninstallPlan& plan,QString& error);
bool repairUninstaller(const QString& package,const QString& directory,QString& error);
}
