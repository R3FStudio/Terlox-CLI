#pragma once
#include <QStringList>
namespace terlox::setup {
struct UninstallPlan { QString directory,version; QStringList files,directories; };
bool buildUninstallPlan(const QString& directory,UninstallPlan& plan,QString& error);
bool uninstallPackage(const QString& directory,QString& error,QStringList& notes,bool removeIntegration=true);
}
