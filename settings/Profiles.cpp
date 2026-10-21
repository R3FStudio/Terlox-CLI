#include "Profiles.h"
#include <QDir>
#include <QStandardPaths>
namespace terlox {
QString defaultWorkingDirectory() {
#ifdef Q_OS_WIN
    return QDir::cleanPath(qEnvironmentVariable("SystemRoot","C:/Windows")+"/System32");
#else
    return QDir::homePath();
#endif
}
QList<Profile> discoverProfiles(const QString& appDir) {
    QList<Profile> out;
#ifdef Q_OS_WIN
    const auto integrated = appDir + "/terloxsh.exe";
    const QStringList names{"pwsh", "powershell", "cmd", "bash"};
#else
    const auto integrated = appDir + "/terloxsh";
    const QStringList names{"bash", "zsh", "fish", "sh"};
#endif
    if (QFileInfo::exists(integrated)) out << Profile{"Terlox shell", {integrated, {}, defaultWorkingDirectory()}};
    for (const auto& n : names) {
        auto exe = QStandardPaths::findExecutable(n);
#ifdef Q_OS_WIN
        if (exe.isEmpty() && n=="powershell") exe=QStandardPaths::findExecutable(n,{qEnvironmentVariable("SystemRoot","C:/Windows")+"/System32/WindowsPowerShell/v1.0"});
#endif
        if (exe.isEmpty()) continue;
        QStringList args;
        if (n == "powershell" || n == "pwsh") args << "-NoLogo";
        else if (n != "cmd") {
#ifdef Q_OS_MACOS
            if (n!="sh") args << "-l";
#endif
            args << "-i";
        }
        out << Profile{n, {exe, args, defaultWorkingDirectory()}};
    }
    return out;
}
}
