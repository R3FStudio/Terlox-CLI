#pragma once
#include "Release.h"
namespace terlox::updates {
struct InstallPlan {
    QString kind,destination,restart;
    QUrl asset;
};
bool planInstallation(const Release& release,const QString& platform,const QString& executable,const QString& appImage,const QString& appDir,InstallPlan& plan,QString& error);
bool launchInstallation(const InstallPlan& plan,const QString& package,const QString& work,qint64 parentPid,QString& error);
}
