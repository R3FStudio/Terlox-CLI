#pragma once
#include <QStringList>
namespace terlox {
struct ElevationRequest { QString executable; QStringList arguments; QString directory; };
struct AdminCommand { QString engine="powershell",command; bool help=false; };
AdminCommand parseAdminCommand(QString command);
ElevationRequest elevatedTerminalRequest(const QString& appDirectory, const QString& command, const QString& directory,const QString& engine={});
ElevationRequest elevatedProgramRequest(const QString& appDirectory,const QString& executable,const QStringList& arguments,const QString& directory);
bool executableRequiresElevation(const QString& executable);
bool elevateTerminal(const ElevationRequest& request, QString& error, quintptr owner=0);
bool isAdministrator();
}
