#include "Builtins.h"
#include "Help.h"
#include "information/Report.h"
#include "integration/defaults/Command.h"
#include "updates/Command.h"
#include <QCoreApplication>
#include <QDir>
#include <QProcessEnvironment>
#include <iostream>
namespace terlox::shell {
static QString builtinName(QString n) {
#ifdef Q_OS_WIN
    n=n.toLower();
    if (n=="chdir" || n=="sl" || n=="set-location") n="cd";
    if (n.size()==2 && n[0].isLetter() && n[1]==':') return "cd";
#endif
    return n;
}
bool isBuiltin(const QString& n) { return QStringList{"cd","pwd","echo","clear","exit","export","set","unset","env","history","help","/info-terlox","terlox-default","terlox-update"}.contains(builtinName(n)); }
int builtin(const QStringList& a, ShellState& state) {
    const auto n = builtinName(a[0]);
    if (n=="help") { std::cout << shellHelp().toUtf8().constData(); return 0; }
    if (n=="terlox-update") return updateCommand(a.mid(1));
    if (n=="terlox-default") return defaultTerminalCommand(a.mid(1),QCoreApplication::applicationDirPath()+
#ifdef Q_OS_WIN
        "/terlox.exe"
#else
        "/terlox"
#endif
    );
    if (n == "/info-terlox") { std::cout << informationReport().toUtf8().constData(); return 0; }
    auto error = [](const QString& e) { std::cerr << "terloxsh: " << e.toUtf8().constData() << '\n'; return 1; };
    if (n == "cd") {
        auto arguments=a.mid(1);
#ifdef Q_OS_WIN
        if (!arguments.isEmpty() && arguments.first().compare("/d",Qt::CaseInsensitive)==0) arguments.removeFirst();
        if (a[0].size()==2 && a[0][1]==':') arguments={a[0]+"/"};
        // CMD accepts spaces in an unquoted cd path and prints cwd with no argument.
        if (arguments.isEmpty()) { std::cout << QDir::toNativeSeparators(QDir::currentPath()).toUtf8().constData() << '\n'; return 0; }
        QString path=arguments.join(' ');
        for (qsizetype start=path.indexOf('%'); start>=0;) {
            const auto end=path.indexOf('%',start+1); if (end<0) break;
            const auto name=path.mid(start+1,end-start-1).toUtf8();
            const auto value=qEnvironmentVariable(name.constData());
            if (qEnvironmentVariableIsSet(name.constData())) { path.replace(start,end-start+1,value); start=path.indexOf('%',start+value.size()); }
            else start=path.indexOf('%',end+1);
        }
        if (path=="~") path=QDir::homePath();
#else
        if (arguments.size()>1) return error("cd: too many arguments");
        const auto path=arguments.value(0,QDir::homePath());
#endif
        return QDir::setCurrent(path) ? 0 : error("cd: directory not accessible: "+path);
    }
    if (n == "pwd") { std::cout << QDir::currentPath().toUtf8().constData() << '\n'; return 0; }
    if (n == "echo") { std::cout << a.mid(1).join(' ').toUtf8().constData() << '\n'; return 0; }
    if (n == "clear") { std::cout << "\033[2J\033[H"; return 0; }
    if (n == "exit") {
        bool valid = true; const int code = a.size() > 1 ? a[1].toInt(&valid) : state.status;
        if (!valid || a.size() > 2) return error("exit: expected one numeric status");
        state.exiting = true; return code & 255;
    }
    if (n == "export" || n == "set") {
        if (a.size() == 1) return builtin({"env"}, state);
        for (const auto& assignment : a.mid(1)) {
            const auto at = assignment.indexOf('=');
            if (at < 1 || assignment.left(at).contains(' ')) return error("export: use NAME=value");
            if (!qputenv(assignment.left(at).toUtf8(), assignment.mid(at + 1).toUtf8())) return error("cannot set environment variable");
        }
        return 0;
    }
    if (n == "unset") { for (const auto& key : a.mid(1)) qunsetenv(key.toUtf8()); return 0; }
    if (n == "env") { for (const auto& item : QProcessEnvironment::systemEnvironment().toStringList()) std::cout << item.toUtf8().constData() << '\n'; return 0; }
    return error("history is available in the interactive shell");
}
}
