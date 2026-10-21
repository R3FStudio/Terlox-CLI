#pragma once
#include <QString>
#include <QStringList>
namespace terlox::setup {
struct InstallOptions {
    QString source, destination;
    bool desktopIntegration=true;
    bool desktopShortcut=false;
    bool defaultTerminal=false;
    bool registerApplication=false;
};
struct InstallPlan {
    InstallOptions options;
    QString version, executable;
    QStringList files;
    qint64 bytes=0;
};
QString defaultDestination();
QString defaultTerminalExplanation();
bool canSelectDefaultTerminal();
bool buildInstallPlan(const InstallOptions& options,InstallPlan& plan,QString& error);
}
