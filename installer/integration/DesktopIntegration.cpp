#include "DesktopIntegration.h"
#include "integration/defaults/DefaultTerminal.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QProcess>
#include <QSet>
namespace terlox::setup {
static bool writeFile(const QString& path,const QByteArray& data,QString& error) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) { error="Cannot create "+path; return false; }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data)!=data.size() || !file.commit()) { error="Cannot write "+path+": "+file.errorString(); return false; } return true;
}
bool registerDesktop(const InstallPlan& plan,QString& error,QStringList& notes) {
    const auto destination=plan.options.destination;
#ifdef Q_OS_WIN
    QProcess registration;
    registration.setProgram(qEnvironmentVariable("SystemRoot","C:/Windows")+"/System32/WindowsPowerShell/v1.0/powershell.exe");
    QStringList args{"-NoLogo","-NoProfile","-NonInteractive","-ExecutionPolicy","Bypass","-File",QDir(destination).filePath("integration/windows/Register-Terlox.ps1"),"-ApplicationDirectory",destination};
    if (plan.options.desktopShortcut) args << "-DesktopShortcut";
    registration.setArguments(args); registration.start();
    if (!registration.waitForStarted() || !registration.waitForFinished(60000)) { registration.kill(); registration.waitForFinished(); error=registration.errorString(); return false; }
    if (registration.exitStatus()!=QProcess::NormalExit || registration.exitCode()!=0) { error=QString::fromUtf8(registration.readAllStandardError()); return false; }
    notes << "Registered Start, Run and Explorer Open in Terlox for this user.";
    if (plan.options.defaultTerminal) { const auto result=selectDefaultTerminal(QDir(destination).filePath(plan.executable)); if (!result.success) { error=result.message; return false; } notes << result.message; }
    return true;
#elif defined(Q_OS_MACOS)
    const auto app=QDir(destination).filePath("terlox.app");
    QProcess registration;
    registration.start("/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister",{"-f",app});
    if (!registration.waitForFinished(30000) || registration.exitCode()!=0) { error="Launch Services could not register the installed app."; return false; }
    if (plan.options.defaultTerminal) { const auto result=selectDefaultTerminal(app+"/Contents/MacOS/terlox"); if (!result.success) { error=result.message; return false; } notes << result.message; }
    notes << "The app is installed in "+app+". "+defaultTerminalExplanation(); return true;
#else
    const auto data=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const auto executable=QDir(destination).filePath(plan.executable);
    auto quote=[](QString value) { value.replace('\\',"\\\\"); value.replace('"',"\\\""); value.replace('`',"\\`"); value.replace('$',"\\$"); value.replace('%',"%%"); return '"'+value+'"'; };
    const auto icon=QDir(destination).filePath("share/icons/hicolor/256x256/apps/app.terlox.terminal.png");
    const auto entry=QString("[Desktop Entry]\nType=Application\nName=Terlox\nComment=Native terminal and shell\nExec=%1\nTryExec=%2\nIcon=%3\nTerminal=false\nCategories=System;TerminalEmulator;\nX-TerminalArgExec=-e\nX-TerminalArgDir=--working-directory\n").arg(quote(executable),executable,icon);
    const auto entryName=QString("app.terlox.terminal.desktop");
    if (!writeFile(data+"/applications/"+entryName,entry.toUtf8(),error)) return false;
    if (plan.options.desktopShortcut) {
        const auto path=QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)+'/'+entryName;
        if (!writeFile(path,entry.toUtf8(),error)) return false;
        QFile::setPermissions(path,QFile::permissions(path)|QFile::ExeOwner);
        notes << "Your desktop may ask you to trust its new launcher.";
    }
    if (plan.options.defaultTerminal) {
        const auto result=selectDefaultTerminal(executable);
        if (!result.success) { error=result.message; return false; }
        notes << result.message;
    }
    notes << "Registered the Terlox application launcher for this user."; return true;
#endif
}
}
