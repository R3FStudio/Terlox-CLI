#include "AdminLaunch.h"
#include <QStandardPaths>
#include <QFileInfo>
namespace terlox {
LaunchOptions nativeAdminLaunch(const QString& engine,const QString& command,const QString& directory,QString& error) {
    error.clear();
    LaunchOptions launch; launch.directory=directory;
#ifdef Q_OS_WIN
    if (engine=="cmd") {
        launch.executable=qEnvironmentVariable("COMSPEC","C:/Windows/System32/cmd.exe");
        launch.nativeArguments=command.isEmpty()?"/d":"/d /s /k \""+command+"\"";
    } else if (engine=="powershell") {
        auto exe=QStandardPaths::findExecutable("pwsh");
        if (exe.isEmpty()) exe=QStandardPaths::findExecutable("powershell");
        if (exe.isEmpty()) exe=qEnvironmentVariable("SystemRoot","C:/Windows")+"/System32/WindowsPowerShell/v1.0/powershell.exe";
        if (!QFileInfo::exists(exe)) { error="PowerShell is unavailable; use admin --cmd."; return {}; }
        launch.executable=exe; launch.arguments={"-NoLogo","-NoExit"};
        if (!command.isEmpty()) {
            const QByteArray utf16(reinterpret_cast<const char*>(command.utf16()),command.size()*2);
            launch.arguments << "-EncodedCommand" << QString::fromLatin1(utf16.toBase64());
        }
    } else { error="Admin engine must be powershell or cmd."; return {}; }
#else
    Q_UNUSED(engine);
    const auto sudo=QStandardPaths::findExecutable("sudo");
    if (sudo.isEmpty()) { error="sudo is not installed; choose a native shell and use the installed su/doas tool."; return {}; }
    auto shell=qEnvironmentVariable("SHELL","/bin/sh"); if (!QFileInfo(shell).isExecutable()) shell="/bin/sh";
    launch.executable=sudo;
    launch.arguments={"--",shell};
    if (command.isEmpty()) launch.arguments << "-l";
    else {
        auto quoted=shell; quoted.replace("'","'\\''");
        launch.arguments << "-c" << command+"\nexec '"+quoted+"' -l";
    }
#endif
    return launch;
}
}
