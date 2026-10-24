#include "Update.h"
#include "installer/backend/InstallEngine.h"
#include "installer/uninstall/UninstallPlan.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <windows.h>

namespace terlox::setup {
bool applyUpdate(const QString& request,const QString& source,QString& error) {
    QFile file(request);
    if (!file.open(QIODevice::ReadOnly) || file.size()>8192) { error="Cannot read the update request."; return false; }
    const auto data=QJsonDocument::fromJson(file.readAll()).object(); file.close();
    const auto directory=data.value("directory").toString();
    const auto pid=data.value("pid").toInteger();
    UninstallPlan previous;
    if (data.value("application")!="app.terlox.terminal" || pid<=0 || pid>MAXDWORD
        || !buildUninstallPlan(directory,previous,error)) { if (error.isEmpty()) error="Invalid update request or installation."; return false; }
    bool success=false;
    HANDLE process=OpenProcess(SYNCHRONIZE,FALSE,DWORD(pid));
    if (process) {
        const auto waited=WaitForSingleObject(process,60000); CloseHandle(process);
        if (waited!=WAIT_OBJECT_0) error="Terlox did not close in time. The update was not installed.";
    } else if (GetLastError()!=ERROR_INVALID_PARAMETER) error="Could not wait for Terlox to close.";
    if (error.isEmpty()) {
        InstallPlan plan;
        if (buildInstallPlan({source,directory,false,false,false,true},plan,error)) {
            const auto result=installPackage(plan); success=result.success; error=result.message;
        }
    }
    const auto report=QFileInfo(request).absolutePath()+"/result.json";
    QSaveFile result(report);
    if (result.open(QIODevice::WriteOnly)) {
        result.write(QJsonDocument(QJsonObject{{"success",success},{"message",error}}).toJson()); result.commit();
    }
    if (data.value("restart").toBool()) {
        QProcess restart; restart.setProgram(QDir(directory).filePath("terlox.exe"));
        restart.setArguments({"--update-result",report}); restart.setWorkingDirectory(directory);
        if (!restart.startDetached()) { error+=" Could not restart Terlox; open it from Start."; return false; }
    }
    return success;
}
}
