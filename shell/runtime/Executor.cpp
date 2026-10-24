#include "Executor.h"
#include "NativeShell.h"
#include "TerminalState.h"
#include "information/About.h"
#include "shell/parsing/Parser.h"
#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <memory>
namespace terlox::shell {
static std::filesystem::path nativePath(const QString& s) {
    const auto utf8 = s.toUtf8();
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(utf8.constData()), size_t(utf8.size())));
}
int execute(const Pipeline& pipeline, ShellState& state) {
    if (pipeline.empty()) return 0;
    const auto& first = pipeline.front();
    if (pipeline.size() == 1 && isBuiltin(first.arguments[0])) {
        // Builtins execute in the parent to preserve cwd, environment and exit state.
        std::ofstream output, error;
        auto* oldOut = std::cout.rdbuf(); auto* oldErr = std::cerr.rdbuf();
        if (!first.input.isEmpty()) { QFile f(first.input); if (!f.open(QIODevice::ReadOnly)) { std::cerr << "terloxsh: cannot open input\n"; return 1; } }
        if (!first.output.isEmpty()) {
            output.open(nativePath(first.output), first.append ? std::ios::app : std::ios::out);
            if (!output) { std::cerr << "terloxsh: cannot open output\n"; return 1; }
            std::cout.rdbuf(output.rdbuf());
        }
        if (!first.error.isEmpty()) {
            error.open(nativePath(first.error));
            if (!error) { std::cout.rdbuf(oldOut); std::cerr << "terloxsh: cannot open stderr file\n"; return 1; }
            std::cerr.rdbuf(error.rdbuf());
        }
        const auto code = builtin(first.arguments, state);
        std::cout.flush(); std::cerr.flush(); std::cout.rdbuf(oldOut); std::cerr.rdbuf(oldErr); return code;
    }
    std::vector<std::unique_ptr<QProcess>> processes;
    TerminalState terminalState;
    for (size_t i = 0; i < pipeline.size(); ++i) {
        const auto& c = pipeline[i]; auto p = std::make_unique<QProcess>();
        auto args = c.arguments; const auto executable = args.takeFirst();
        if (isBuiltin(executable)) { args.prepend(executable); args.prepend("--builtin"); p->setProgram(QCoreApplication::applicationFilePath()); }
        else configureNativeProgram(*p,executable,args);
        if (isBuiltin(executable)) p->setArguments(args);
        const bool forwardOutput = i + 1 == pipeline.size() && c.output.isEmpty();
        const bool forwardError = c.error.isEmpty();
        p->setProcessChannelMode(forwardOutput ? (forwardError ? QProcess::ForwardedChannels : QProcess::ForwardedOutputChannel)
                                              : (forwardError ? QProcess::ForwardedErrorChannel : QProcess::SeparateChannels));
        if (i == 0 && c.input.isEmpty()) p->setInputChannelMode(QProcess::ForwardedInputChannel);
        if (!c.input.isEmpty()) p->setStandardInputFile(c.input);
        if (!c.output.isEmpty()) p->setStandardOutputFile(c.output, c.append ? QIODevice::Append : QIODevice::Truncate);
        if (!c.error.isEmpty()) p->setStandardErrorFile(c.error);
#ifndef Q_OS_WIN
        p->setUnixProcessParameters(QProcess::UnixProcessFlag::ResetSignalHandlers);
#endif
        processes.push_back(std::move(p));
    }
    for (size_t i = 0; i + 1 < processes.size(); ++i) {
        if (!pipeline[i].output.isEmpty() || !pipeline[i + 1].input.isEmpty()) { std::cerr << "terloxsh: pipe conflicts with redirection\n"; return 2; }
        processes[i]->setStandardOutputProcess(processes[i + 1].get());
    }
    for (auto& p : processes) {
        p->start();
        if (!p->waitForStarted()) {
            std::cerr << "terloxsh: " << p->program().toUtf8().constData() << ": " << p->errorString().toUtf8().constData() << '\n';
            for (auto& other : processes) if (other->state() != QProcess::NotRunning) { other->kill(); other->waitForFinished(); }
            return 127;
        }
    }
    for (auto& p : processes) p->waitForFinished(-1);
    return processes.back()->exitStatus() == QProcess::CrashExit ? 130 : processes.back()->exitCode();
}
int run(const QString& line, ShellState& state) {
    if (line.simplified().compare("About Terlox",Qt::CaseInsensitive)==0) { auto page=aboutPage(90,30,0,0); page.replace("\033[?1049h",""); std::cout << page.constData() << "\033[0m\033[?25h\r\n" << std::flush; return state.status=0; }
    if (const auto native = nativeShellLine(line)) return state.status = *native;
    // Expand each statement after earlier statements have updated environment/status.
    QStringList statements; qsizetype start = 0; QChar quote;
    for (qsizetype i = 0; i < line.size(); ++i) {
        const auto c = line[i];
        if (c == '\\' && quote != '\'' && i + 1 < line.size() && QString("\\\"' $|;<>\t").contains(line[i+1])
#ifdef Q_OS_WIN
            && quote!='"'
#endif
        ) { ++i; continue; }
        if (!quote.isNull()) { if (c == quote) quote = QChar(); continue; }
        if (c == '\'' || c == '"') quote = c;
        else if (c == '#' && (i == 0 || line[i-1].isSpace())) break;
        else if (c == ';') { statements << line.mid(start, i-start); start = i+1; }
    }
    statements << line.mid(start);
    for (const auto& statement : statements) {
        if (const auto native = nativeShellLine(statement)) { state.status = *native; continue; }
        const auto parsed = parse(statement, state.status);
        if (!parsed.error.isEmpty()) { std::cerr << "terloxsh: " << parsed.error.toUtf8().constData() << '\n'; return state.status = 2; }
        for (const auto& pipeline : parsed.program) { state.status = execute(pipeline, state); if (state.exiting) return state.status; }
    }
    return state.status;
}
}
