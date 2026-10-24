#include "runtime/Executor.h"
#include "editing/LineEditor.h"
#include "runtime/Help.h"
#include "information/Version.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <iostream>
#include <csignal>
#ifndef Q_OS_WIN
#include "platform/posix/Signals.h"
#endif
#ifdef Q_OS_WIN
#include <windows.h>
static BOOL WINAPI interrupt(DWORD event) { return event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT; }
#endif
int main(int argc, char** argv) {
#ifndef Q_OS_WIN
    if (!terlox::resetTerminalSignals()) { std::cerr << "terloxsh: cannot initialize process signals\n"; return 126; }
#endif
    QCoreApplication app(argc, argv); app.setApplicationName("Terlox"); app.setOrganizationName("Terlox");
#ifdef Q_OS_WIN
    SetConsoleCP(CP_UTF8); SetConsoleOutputCP(CP_UTF8);
    DWORD mode; const auto out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleMode(out, &mode)) SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleCtrlHandler(interrupt, TRUE);
#else
    std::signal(SIGINT, [](int) {});
#endif
    using namespace terlox::shell; ShellState state; const auto args = app.arguments();
    if (args.value(1)=="--help" || args.value(1)=="-h") { std::cout << shellHelp().toUtf8().constData(); return 0; }
    if (args.value(1)=="--version") { std::cout << TERLOX_VERSION << '\n'; return 0; }
    const bool commandAndStay=args.value(1)=="--command-and-stay";
    if (args.value(1) == "--builtin") return builtin(args.mid(2), state);
    if (args.value(1) == "-c") return run(args.value(2), state);
    if (commandAndStay) run(args.value(2),state);
    QFile script;
    if (args.size() > 1 && !commandAndStay) { script.setFileName(args[1]); if (!script.open(QIODevice::ReadOnly)) { std::cerr << "terloxsh: cannot open script\n"; return 1; } }
    if (script.isOpen() || !isInteractive()) {
        QFile input; if (!script.isOpen()) input.open(stdin, QIODevice::ReadOnly);
        auto& stream = script.isOpen() ? script : input;
        while (!stream.atEnd() && !state.exiting) run(QString::fromUtf8(stream.readLine()), state);
        return state.status;
    }
    const auto config = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation); QDir().mkpath(config);
    const auto historyPath = qEnvironmentVariable("TERLOX_HISTORY_FILE", config + "/history"); History history; history.load(historyPath);
    while (!state.exiting) {
        const auto line = readLine(history); if (!line) break;
        if (*line == "history") { for (const auto& entry : history.entries()) std::cout << entry.toUtf8().constData() << '\n'; }
        else run(*line, state);
        history.add(*line); history.save(historyPath);
    }
    return state.status;
}
