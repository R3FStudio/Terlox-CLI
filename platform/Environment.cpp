#include "Environment.h"
#include <QDir>
#include <QFile>
#include <QSet>
#include <QSettings>
#include <QRegularExpression>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace terlox {
QProcessEnvironment terminalEnvironment() {
    auto environment=QProcessEnvironment::systemEnvironment();
    auto paths=environment.value("PATH").split(QDir::listSeparator());
    QSet<QString> seen;
    auto identity=[](QString path) {
        path=QDir::cleanPath(QDir::fromNativeSeparators(path));
#ifdef Q_OS_WIN
        path=path.toCaseFolded();
#endif
        return path;
    };
    for (const auto& path:paths) seen.insert(identity(path));
    auto append=[&](QString path) {
        path=path.trimmed(); if (path.startsWith('"') && path.endsWith('"')) path=path.mid(1,path.size()-2);
        if (!path.isEmpty() && !seen.contains(identity(path))) { paths << path; seen.insert(identity(path)); }
    };
#ifdef Q_OS_WIN
    // Keep the launcher/developer environment first; supplement it with newly
    // registered OS tools instead of replacing explicit inherited overrides.
    auto expand=[&](QString value) {
        const QRegularExpression variable("%([^%]+)%");
        for (int pass=0;pass<8;++pass) {
            auto matches=variable.globalMatch(value); QString expanded; qsizetype previous=0;
            while (matches.hasNext()) { const auto match=matches.next(); expanded+=value.mid(previous,match.capturedStart()-previous);
                expanded+=environment.contains(match.captured(1))?environment.value(match.captured(1)):match.captured(); previous=match.capturedEnd(); }
            expanded+=value.mid(previous); if (expanded==value) break; value=expanded;
        }
        const auto native=value.toStdWString(); const auto count=ExpandEnvironmentStringsW(native.c_str(),nullptr,0);
        if (!count || count>1024*1024) return value;
        std::wstring expanded(count,L'\0'); ExpandEnvironmentStringsW(native.c_str(),expanded.data(),count);
        return QString::fromWCharArray(expanded.c_str());
    };
    for (const auto& root:{QStringLiteral("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),QStringLiteral("HKEY_CURRENT_USER\\Environment")}) {
        QSettings registered(root,QSettings::NativeFormat);
        for (const auto& name:registered.childKeys()) {
            const auto value=expand(registered.value(name).toString());
            if (name.compare("PATH",Qt::CaseInsensitive)==0) { for (const auto& path:value.split(';')) append(path); }
            else if (!environment.contains(name)) environment.insert(name,value);
        }
    }
#elif defined(Q_OS_MACOS)
    auto pathFile=[&](const QString& name) {
        QFile file(name); if (file.open(QIODevice::ReadOnly)) for (const auto& line:QString::fromUtf8(file.readAll()).split('\n')) if (!line.startsWith('#')) append(line);
    };
    pathFile("/etc/paths"); QDir additions("/etc/paths.d");
    for (const auto& name:additions.entryList(QDir::Files,QDir::Name)) pathFile(additions.filePath(name));
    for (const auto& path:{QString("/opt/homebrew/bin"),QString("/opt/homebrew/sbin"),QDir::homePath()+"/.local/bin"}) if (QDir(path).exists()) append(path);
#else
    for (const auto& path:{QString("/usr/local/bin"),QDir::homePath()+"/.local/bin"}) if (QDir(path).exists()) append(path);
#endif
    environment.insert("PATH",paths.join(QDir::listSeparator())); return environment;
}
QProcessEnvironment interactiveTerminalEnvironment() {
    auto environment=terminalEnvironment();
    // Advertise this PTY's capabilities while preserving the user's explicit
    // color preferences. No application-specific environment rules are needed.
    environment.insert("TERM","xterm-256color");
    environment.insert("COLORTERM","truecolor");
    return environment;
}
void importTerminalEnvironment() {
    const auto environment=terminalEnvironment();
    const auto inherited=QProcessEnvironment::systemEnvironment();
    for (const auto& name:environment.keys()) if (!inherited.contains(name) || inherited.value(name)!=environment.value(name)) {
#ifdef Q_OS_WIN
        _wputenv_s(name.toStdWString().c_str(),environment.value(name).toStdWString().c_str());
#else
        qputenv(name.toUtf8(),environment.value(name).toUtf8());
#endif
    }
}
}
