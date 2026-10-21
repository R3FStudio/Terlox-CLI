#include "Elevation.h"
namespace terlox {
AdminCommand parseAdminCommand(QString command) {
    AdminCommand result; result.command=command.trimmed(); result.help=result.command=="--help";
    for (const auto& prefix:{QString("--cmd"),QString("cmd"),QString("--powershell"),QString("ps")}) {
        if (result.command==prefix || result.command.startsWith(prefix+' ')) {
            result.engine=(prefix=="cmd" || prefix=="--cmd")?"cmd":"powershell";
            result.command=result.command.mid(prefix.size()).trimmed(); break;
        }
    }
    return result;
}
ElevationRequest elevatedTerminalRequest(const QString& appDirectory, const QString& command, const QString& directory,const QString& engine) {
    ElevationRequest request{appDirectory+"/terlox.exe",{"--directory",directory},directory};
    if (!command.isEmpty()) request.arguments << "--admin-command" << QString::fromLatin1(command.toUtf8().toBase64());
    request.arguments << "--admin-engine" << (engine.isEmpty()?"powershell":engine);
    return request;
}
ElevationRequest elevatedProgramRequest(const QString& appDirectory,const QString& executable,const QStringList& arguments,const QString& directory) {
    return {appDirectory+"/terlox.exe",QStringList{"--directory",directory,"-e",executable}+arguments,directory};
}
#ifndef Q_OS_WIN
bool elevateTerminal(const ElevationRequest&, QString& error, quintptr) { error="Windows UAC is unavailable on this OS. Use the native sudo/su command."; return false; }
bool isAdministrator() { return false; }
bool executableRequiresElevation(const QString&) { return false; }
#endif
}
