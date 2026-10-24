#include "NativeShell.h"
#include "TerminalState.h"
#include "Builtins.h"
#include "shell/parsing/Parser.h"
#include "platform/Elevation.h"
#include "platform/AdminLaunch.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QRegularExpression>
#include <iostream>
namespace terlox::shell {
#ifdef Q_OS_WIN
static QString powershell() {
    auto exe = QStandardPaths::findExecutable("pwsh");
    if (exe.isEmpty()) exe=QStandardPaths::findExecutable("powershell");
    if (exe.isEmpty()) exe=QStandardPaths::findExecutable("powershell",{qEnvironmentVariable("SystemRoot","C:/Windows")+"/System32/WindowsPowerShell/v1.0"});
    return exe;
}

#endif
static int wait(QProcess& p) {
    TerminalState terminalState;
#ifndef Q_OS_WIN
    p.setUnixProcessParameters(QProcess::UnixProcessFlag::ResetSignalHandlers);
#endif
    p.setProcessChannelMode(QProcess::ForwardedChannels); p.setInputChannelMode(QProcess::ForwardedInputChannel);
    p.start();
    if (!p.waitForStarted()) { std::cerr << "terloxsh: " << p.errorString().toUtf8().constData() << '\n'; return 127; }
    p.waitForFinished(-1); return p.exitStatus() == QProcess::CrashExit ? 130 : p.exitCode();
}
std::optional<int> nativeShellLine(const QString& line,const ElevationLauncher& launcher) {
    const auto match = QRegularExpression("^\\s*([^\\s]+)(?:\\s+([\\s\\S]*))?$").match(line);
    if (!match.hasMatch()) return {};
    const auto name = match.captured(1); const auto code = match.captured(2);
#ifdef Q_OS_WIN
    const bool sudo=name.compare("sudo",Qt::CaseInsensitive)==0;
    auto launchElevated=[&](const ElevationRequest& request) {
        QString error;
        const bool opened=launcher?launcher(request,error):elevateTerminal(request,error);
        if (!opened) { std::cerr << error.toStdString() << '\n'; return 1; }
        std::cout << "Opened elevated native terminal.\n"; return 0;
    };
    auto requestElevation=[&](const QString& command,const QString& engine) {
        return launchElevated(elevatedTerminalRequest(QCoreApplication::applicationDirPath(),command,QDir::currentPath(),engine));
    };
#else
    Q_UNUSED(launcher);
    const bool sudo=false;
#endif
    if (name.compare("admin",Qt::CaseInsensitive)==0 || sudo) {
#ifdef Q_OS_WIN
        if (sudo && (code.trimmed().isEmpty() || code=="--help" || code=="-h")) {
            std::cout << "sudo [--cmd|--powershell] command: request Windows UAC for a command.\n"
                         "sudo -i: open an administrator shell. Cancel UAC to leave the command unrun.\n";
            return code.trimmed().isEmpty()?2:0;
        }
        if (sudo && (code=="-i" || code=="-s")) return requestElevation({},"powershell");
#endif
        const auto admin=parseAdminCommand(code);
        if (admin.help) { std::cout << "admin [--powershell|--cmd] [command]: native elevated shell. Windows requests UAC; Unix sudo prompts in this terminal.\n"; return 0; }
        QString error;
#ifdef Q_OS_WIN
        if (sudo && admin.command.startsWith('-')) {
            std::cerr << "Unsupported sudo option. Use sudo --help for Windows options.\n"; return 2;
        }
        return requestElevation(admin.command,admin.engine);
#else
        const auto launch=nativeAdminLaunch({},admin.command,QDir::currentPath(),error);
        if (!error.isEmpty()) { std::cerr << error.toStdString() << '\n'; return 1; }
        QProcess process; process.setProgram(launch.executable); process.setArguments(launch.arguments); return wait(process);
#endif
    }
#ifdef Q_OS_WIN
    if (isBuiltin(name)) return {}; // Location/environment changes belong to the parent.
    const bool cmdBuiltin = QStringList{"dir","type","copy","del","erase","ren","rename","mkdir","rmdir","md","rd","cls","ver","vol","title","start"}.contains(name,Qt::CaseInsensitive);
    const bool cmdlet = QRegularExpression("^[A-Za-z]+-[A-Za-z][A-Za-z0-9]*$").match(name).hasMatch()
        && QStandardPaths::findExecutable(name).isEmpty() && !QFileInfo::exists(name);
    const bool psAlias=QStringList{"ls","cat","cp","mv","rm","man","gci","gi","gc","gcm","gsv","gps","select","where","sort","measure"}.contains(name,Qt::CaseInsensitive)
        && QStandardPaths::findExecutable(name).isEmpty() && !QFileInfo::exists(name);
    const auto tokens=QProcess::splitCommand(line);
    const auto executable=QStandardPaths::findExecutable(tokens.value(0));
    // CMD owns its complete builtin language. Let the native engine resolve
    // names absent from PATH instead of limiting it to our convenience aliases.
    const bool nativeCmdFallback=executable.isEmpty() && !QFileInfo::exists(tokens.value(0))
        && !cmdlet && !psAlias && name.compare("ps",Qt::CaseInsensitive)!=0;
    if (!executable.isEmpty() && !isAdministrator() && executableRequiresElevation(executable)) {
        const auto parsed=parse(line);
        if (parsed.error.isEmpty() && parsed.program.size()==1 && parsed.program[0].size()==1) {
            const auto& command=parsed.program[0][0];
            if (command.input.isEmpty() && command.output.isEmpty() && command.error.isEmpty())
                return launchElevated(elevatedProgramRequest(QCoreApplication::applicationDirPath(),executable,command.arguments.mid(1),QDir::currentPath()));
        }
        std::cerr << "This command requires administrator rights. Use sudo --cmd with the complete native command for pipes or redirection.\n";
        return 5;
    }
    QProcess p;
    if (name.compare("ps",Qt::CaseInsensitive) == 0 || cmdlet || psAlias) {
        p.setProgram(powershell());
        if (p.program().isEmpty()) { std::cerr << "terloxsh: install PowerShell or select an installed shell profile\n"; return 127; }
        const auto script = cmdlet || psAlias ? line : code;
        if (script.trimmed().isEmpty()) { p.setArguments({"-NoLogo"}); return wait(p); }
        const QByteArray utf16(reinterpret_cast<const char*>(script.utf16()),script.size()*2);
        p.setArguments({"-NoLogo","-NoProfile","-EncodedCommand",QString::fromLatin1(utf16.toBase64())});
        return wait(p);
    }
    if (name.compare("cmd",Qt::CaseInsensitive) == 0 || cmdBuiltin || nativeCmdFallback) {
        p.setProgram(qEnvironmentVariable("COMSPEC","C:/Windows/System32/cmd.exe"));
        const auto script = cmdBuiltin || nativeCmdFallback ? line : code;
        // CMD's /s /c command-string quoting is distinct from MSVC argv quoting.
        if (script.trimmed().isEmpty()) p.setArguments({"/d"});
        else if (!cmdBuiltin && script.startsWith('/')) p.setNativeArguments(script);
        else p.setNativeArguments("/d /s /c \"" + script + "\"");
        return wait(p);
    }
#else
    Q_UNUSED(line);
#endif
    return {};
}
void configureNativeProgram(QProcess& p, const QString& requested, const QStringList& arguments) {
    QString executable=requested;
#ifdef Q_OS_WIN
    // CreateProcess only appends .exe. Resolve PATHEXT first for .com tools
    // (more, tree) and for extensionless .cmd/.bat invocations on PATH.
    const auto resolved=QStandardPaths::findExecutable(requested);
    if (!resolved.isEmpty()) executable=resolved;
    const auto extension = QFileInfo(executable).suffix().toLower();
    if (extension == "ps1") { p.setProgram(powershell()); p.setArguments(QStringList{"-NoLogo","-NoProfile","-File",executable} + arguments); return; }
    if (extension == "cmd" || extension == "bat") {
        p.setProgram(qEnvironmentVariable("COMSPEC","C:/Windows/System32/cmd.exe"));
        auto quote = [](QString s) { return "\"" + s.replace('"',"\"\"") + "\""; };
        QString command = quote(executable); for (const auto& a : arguments) command += ' ' + quote(a);
        p.setArguments({}); p.setNativeArguments("/d /s /c \"" + command + "\""); return;
    }
#endif
    p.setProgram(executable); p.setArguments(arguments);
}
}
