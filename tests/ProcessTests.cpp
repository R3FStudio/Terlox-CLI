#include "platform/Process.h"
#include "platform/AdminLaunch.h"
#include "platform/Environment.h"
#include "terminal/Screen.h"
#include "TerminalClientFixture.h"
#include <QCoreApplication>
#include <QDir>
#include <QBuffer>
#include <QSignalSpy>
#include <QProcess>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QProcessEnvironment>
#include <QScopeGuard>
#include <QtTest>
#include <iostream>
#include <thread>
#include <chrono>
#ifdef Q_OS_WIN
#include <io.h>
#else
#include <unistd.h>
#endif
using namespace terlox;
class ProcessTests : public QObject {
    Q_OBJECT
    QString shell() const {
#ifdef Q_OS_WIN
        return QCoreApplication::applicationDirPath() + "/terloxsh.exe";
#else
        return QCoreApplication::applicationDirPath() + "/terloxsh";
#endif
    }
    static QString content(Screen& s) { QStringList lines; for (int row = 0; row < s.rows(); ++row) lines << s.text(row,0,s.columns()-1); return lines.join('\n'); }
private slots:
    void interactiveEnvironmentPreservesColorPreferences() {
        const auto oldTerm=qgetenv("TERM"),oldColor=qgetenv("NO_COLOR");
        const auto restore=qScopeGuard([&] { for (const auto& pair:{qMakePair("TERM",oldTerm),qMakePair("NO_COLOR",oldColor)}) { if (pair.second.isNull()) qunsetenv(pair.first); else qputenv(pair.first,pair.second); } });
        qputenv("TERM","dumb"); qputenv("NO_COLOR","1");
        auto environment=interactiveTerminalEnvironment();
        QCOMPARE(environment.value("TERM"),QString("xterm-256color")); QCOMPARE(environment.value("COLORTERM"),QString("truecolor"));
        QCOMPARE(environment.value("NO_COLOR"),QString("1"));
    }
    void shellChildrenKeepTheirTerminal() {
        Process process; Screen screen(10,100); QString error;
        connect(&process,&Process::received,&screen,&Screen::feed);
        const auto command="\""+QCoreApplication::applicationFilePath()+"\" --tty-child";
        QVERIFY2(process.start({shell(),{"-c",command},QDir::tempPath(),10,100},error),qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains("tty-in=1 tty-out=1 tty-err=1"),10000);
    }
    void nativePasswordInputDoesNotEcho() {
        Process process; Screen screen(10,100); QString error;
        connect(&process,&Process::received,&screen,&Screen::feed);
        const auto command="\""+QCoreApplication::applicationFilePath()+"\" --hidden-input-child";
        QVERIFY2(process.start({shell(),{"-c",command},QDir::tempPath(),10,100},error),qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains("Password input probe:"),5000);
#ifdef Q_OS_WIN
        process.write("terlox-dummy-secret\r");
#else
        process.write("terlox-dummy-secret\n");
#endif
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains("secret-match=1"),5000);
        QVERIFY(!content(screen).contains("terlox-dummy-secret"));
        for (int offset=0;offset<=screen.historySize();++offset)
            for (int row=0;row<screen.rows();++row)
                QVERIFY(!screen.text(row,0,screen.columns()-1,offset).contains("terlox-dummy-secret"));
    }
    void terminalPaletteRepliesReachNativeChildren_data() {
        QTest::addColumn<QString>("executable");
        QTest::addColumn<QStringList>("arguments");
        const auto client=QCoreApplication::applicationFilePath();
        QTest::newRow("direct") << client << QStringList{"--palette-query-child"};
        QTest::newRow("terlox-shell") << shell() << QStringList{"-c","\""+client+"\" --palette-query-child"};
#ifndef Q_OS_WIN
        auto quoted=client; quoted.replace("'","'\\''");
        const auto command="'"+quoted+"' --palette-query-child";
        QTest::newRow("system-shell") << QString("/bin/sh") << QStringList{"-c",command};
        for (const auto& name:{QString("bash"),QString("zsh"),QString("fish")}) {
            const auto executable=QStandardPaths::findExecutable(name);
            if (!executable.isEmpty()) QTest::newRow(qPrintable(name)) << executable << QStringList{"-c",command};
        }
#endif
    }
    void terminalPaletteRepliesReachNativeChildren() {
        QFETCH(QString,executable); QFETCH(QStringList,arguments);
        Process process; Screen screen(10,120); QString error;
        QSet<QRgb> animationColors;
        connect(&screen,&Screen::changed,this,[&] {
            const auto cell=screen.cell(2,0);
            if (cell.chars[0]=='L') animationColors.insert(screen.color(cell.fg).rgb());
        });
        connect(&process,&Process::received,&screen,&Screen::feed);
        connect(&screen,&Screen::output,&process,&Process::write);
        QVERIFY2(process.start({executable,arguments,QDir::tempPath(),10,120},error),qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains("palette-query-finished"),5000);
        QVERIFY2(content(screen).contains("palette-fg=1 palette-bg=1"),qPrintable(content(screen)));
        QCOMPARE(screen.color(screen.cell(0,119).bg),QColor(40,48,56));
        const auto tail=screen.cell(0,119); QVERIFY(!VTERM_COLOR_IS_DEFAULT_BG(&tail.bg));
        QCOMPARE(tail.attrs.background_alpha,255u);
        QVERIFY(content(screen).contains("term=xterm-256color color=truecolor"));
        QVERIFY(animationColors.size()>=3);
        const auto first=screen.cell(2,0),second=screen.cell(2,1);
        QVERIFY(first.attrs.bold); QVERIFY(second.attrs.bold);
        QCOMPARE(screen.color(first.fg),QColor(210,120,220));
        QCOMPARE(screen.color(second.fg),QColor(40,110,160));
    }
#ifdef Q_OS_WIN
    void powershellProgressIsVisible() {
        const auto powershell=QStandardPaths::findExecutable("powershell");
        if (powershell.isEmpty()) QSKIP("Windows PowerShell is not installed");
        Process process; Screen screen(24,100); QString error; bool progressSeen=false;
        connect(&process,&Process::received,&screen,&Screen::feed);
        connect(&screen,&Screen::changed,this,[&] { if (content(screen).contains("TERLOX_PROGRESS_PROBE")) progressSeen=true; });
        const QString script="$ProgressPreference='Continue'; foreach ($n in 20,50,80) { Write-Progress -Activity 'TERLOX_PROGRESS_PROBE' -Status 'Downloading' -PercentComplete $n; Start-Sleep -Milliseconds 180 }; Write-Progress -Activity 'TERLOX_PROGRESS_PROBE' -Completed; Write-Output 'progress-done'";
        QVERIFY2(process.start({powershell,{"-NoLogo","-NoProfile","-Command",script},QDir::tempPath(),24,100},error),qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains("progress-done"),10000);
        QVERIFY2(progressSeen,"PowerShell Write-Progress must reach the visible screen while running");
    }
#endif
    void inheritedEnvironmentAndPathCommand() {
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
#ifdef Q_OS_WIN
        const auto command=temporary.filePath("terlox-path-probe.exe");
#else
        const auto command=temporary.filePath("terlox-path-probe");
#endif
        QVERIFY(QFile::copy(QCoreApplication::applicationFilePath(),command));
        QVERIFY(QFile::setPermissions(command,QFile::permissions(command)|QFile::ExeOwner|QFile::ExeGroup|QFile::ExeOther));
        const auto previousPath=qgetenv("PATH"),previousProbe=qgetenv("TERLOX_ENV_PROBE");
        const auto restore=qScopeGuard([&] { qputenv("PATH",previousPath); if (previousProbe.isNull()) qunsetenv("TERLOX_ENV_PROBE"); else qputenv("TERLOX_ENV_PROBE",previousProbe); });
        qputenv("PATH",temporary.path().toUtf8()+QDir::listSeparator().toLatin1()+previousPath);
        qputenv("TERLOX_ENV_PROBE","inherited-command-ok");
        Process process; Screen screen(24,100); QString error;
        connect(&process,&Process::received,&screen,&Screen::feed);
        QVERIFY2(process.start({shell(),{"-c","terlox-path-probe --environment-child"},temporary.path(),24,100},error),qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains("inherited-command-ok"),10000);
    }
    void installedGitOnPath() {
        if (QStandardPaths::findExecutable("git").isEmpty()) QSKIP("Git is not installed on this runner");
        Process process; Screen screen(24,100); QString error;
        connect(&process,&Process::received,&screen,&Screen::feed);
        QVERIFY2(process.start({shell(),{"-c","git --version"},QDir::tempPath(),24,100},error),qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains("git version "),10000);
    }
    void wrappedPromptEditing() {
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        const auto directory=temporary.filePath("a long working directory for wrapped prompt editing"); QVERIFY(QDir().mkpath(directory));
        Process process; Screen screen(24,40); QString error;
        connect(&process,&Process::received,&screen,&Screen::feed); connect(&screen,&Screen::output,&process,&Process::write);
        QVERIFY2(process.start({shell(),{},directory,24,40},error),qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains('>'),10000);
        process.write("echo wrapped-command-ox\bk\r");
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).split('\n').contains("wrapped-command-ok"),10000);
        process.stop();
    }
    void clearingInputPreservesQueuedCommands() {
        Process process; Screen screen(12,100); QString error;
        connect(&process,&Process::received,&screen,&Screen::feed);
        connect(&screen,&Screen::output,&process,&Process::write);
        QVERIFY2(process.start({shell(),{},QDir::tempPath(),12,100},error),qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(screen.editableInput().active(),10000);
        for (int i=0;i<3;++i) {
            const auto marker=QString("queued-command-%1-ok").arg(i);
            process.write("echo must-be-cleared");
            process.write(QByteArray(1,char(21))+"echo "+marker.toUtf8()+"\r");
            QTRY_VERIFY_WITH_TIMEOUT(content(screen).split('\n').contains(marker),10000);
            QVERIFY(!content(screen).split('\n').contains("must-be-cleared"));
            QTRY_VERIFY_WITH_TIMEOUT(screen.editableInput().active() && screen.editableInput().empty(),10000);
        }
    }
    void protocolPassthrough() {
        Process process; Screen screen(24,80); QString error;
        connect(&process,&Process::received,&screen,&Screen::feed); connect(&screen,&Screen::output,&process,&Process::write);
        QSignalSpy controls(&screen,&Screen::applicationControls);
        QVERIFY2(process.start({QCoreApplication::applicationFilePath(),{"--protocol-child"},QDir::tempPath(),24,80},error),qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(controls.size()>0,10000);
        QTRY_VERIFY_WITH_TIMEOUT(!screen.graphics().isEmpty(),10000);
        QCOMPARE(screen.graphics()[0].image.pixelColor(0,0),QColor("#12abef"));
        QTRY_COMPARE(screen.text(0,0,2),QString("RGA"));
        QCOMPARE(screen.color(screen.cell(0,0).fg),QColor(Qt::red));
        QCOMPARE(screen.color(screen.cell(0,1).fg),QColor(Qt::green));
        QCOMPARE(screen.cell(0,2).attrs.foreground_alpha,64u);
    }
    void externalExecution() {
        QProcess p; p.start(shell(), {"-c", "echo \"a b\""}); QVERIFY(p.waitForFinished(10000)); QCOMPARE(p.exitCode(),0); QCOMPARE(p.readAllStandardOutput().trimmed(), QByteArray("a b"));
    }
    void redirectAndPipeline() {
        QTemporaryDir temp; QVERIFY(temp.isValid()); QProcess p; p.setWorkingDirectory(temp.path());
#ifdef Q_OS_WIN
        const QString filter = "findstr hello";
#else
        const QString filter = "grep hello";
#endif
        p.start(shell(), {"-c", "echo hello | " + filter + " > result.txt"}); QVERIFY(p.waitForFinished(10000)); QCOMPARE(p.exitCode(),0);
        QFile f(temp.filePath("result.txt")); QVERIFY(f.open(QIODevice::ReadOnly)); QCOMPARE(f.readAll().trimmed(), QByteArray("hello"));
    }
    void stateAndExit() {
        QTemporaryDir temp; QProcess p; p.setWorkingDirectory(temp.path());
        p.start(shell(), {"-c", "export TERLOX_CHILD=value; echo $TERLOX_CHILD; pwd; exit 23"}); QVERIFY(p.waitForFinished(10000)); QCOMPARE(p.exitCode(),23); const auto output = p.readAllStandardOutput(); QVERIFY(output.contains("value")); QVERIFY(output.contains(QDir::fromNativeSeparators(temp.path()).toUtf8()));
    }
    void nativeInteractive() {
        Process process; Screen screen(24,80); QString error;
        connect(&process, &Process::received, &screen, &Screen::feed);
        connect(&screen, &Screen::output, &process, &Process::write);
        QSignalSpy exit(&process, &Process::exited);
        QVERIFY2(process.start({shell(), {}, QDir::tempPath(),24,80}, error), qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains('>'), 10000);
        process.write("echo native-session-ok\r");
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains("native-session-ok\n"), 10000);
        process.resize(30,100); screen.resize(30,100);
        process.write("exit 17\r");
        QTRY_COMPARE_WITH_TIMEOUT(exit.size(), 1, 10000); QCOMPARE(exit.at(0).at(0).toInt(),17);
        process.stop();
    }
    void systemShell() {
        Process process; Screen screen(24,80); QString error;
        connect(&process, &Process::received, &screen, &Screen::feed); connect(&screen, &Screen::output, &process, &Process::write);
        QSignalSpy exit(&process, &Process::exited);
#ifdef Q_OS_WIN
        LaunchOptions launch{qEnvironmentVariable("COMSPEC", "C:/Windows/System32/cmd.exe"), {"/d", "/c", "echo system-shell-ok"}, QDir::tempPath(),24,80};
#else
        LaunchOptions launch{"/bin/sh", {"-c", "echo system-shell-ok"}, QDir::tempPath(),24,80};
#endif
        QVERIFY2(process.start(launch, error), qPrintable(error));
        QTRY_COMPARE_WITH_TIMEOUT(exit.size(),1,10000);
        QVERIFY2(content(screen).contains("system-shell-ok"), qPrintable(content(screen)));
    }
#ifdef Q_OS_WIN
    void nativeAdminCommandEngines() {
        for (const auto& engine : {QString("cmd"),QString("powershell")}) {
            Process process; Screen screen(24,100); QString error;
            connect(&process,&Process::received,&screen,&Screen::feed);
            connect(&screen,&Screen::output,&process,&Process::write);
            QSignalSpy exited(&process,&Process::exited);
            // Exercise the native engine/quoting through ConPTY without requesting UAC.
            const auto code=engine=="cmd" ? QString("echo native-cmd-ok & exit 13")
                : QString("@(1,2,3) | ForEach-Object { $_ * 2 } | ForEach-Object { Write-Output ('native-ps-' + $_) }; exit 14");
            auto launch=nativeAdminLaunch(engine,code,QDir::tempPath(),error);
            QVERIFY2(error.isEmpty(),qPrintable(error));
            QVERIFY2(process.start(launch,error),qPrintable(error));
            QTRY_COMPARE_WITH_TIMEOUT(exited.size(),1,15000);
            QCOMPARE(exited[0][0].toInt(),engine=="cmd"?13:14);
            QVERIFY2(content(screen).contains(engine=="cmd"?"native-cmd-ok":"native-ps-6"),qPrintable(content(screen)));
        }
    }
    void powershellSession() {
        const auto executable=QStandardPaths::findExecutable("powershell"); QVERIFY(!executable.isEmpty());
        Process process; Screen screen(24,100); QString error;
        connect(&process,&Process::received,&screen,&Screen::feed); connect(&screen,&Screen::output,&process,&Process::write);
        QSignalSpy exit(&process,&Process::exited);
        QVERIFY2(process.start({executable,{"-NoLogo","-NoProfile"},QDir::tempPath(),24,100},error),qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains("PS "),10000);
        process.write("Write-Output powershell-session-ok\r");
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains("powershell-session-ok\n"),10000);
        process.write("exit 12\r"); QTRY_COMPARE_WITH_TIMEOUT(exit.size(),1,10000); QCOMPARE(exit[0][0].toInt(),12);
    }
#endif
    void shutdown() {
        Process process; Screen screen(24,80); QString error;
        connect(&process, &Process::received, &screen, &Screen::feed); connect(&screen, &Screen::output, &process, &Process::write);
        QVERIFY2(process.start({shell(), {}, QDir::tempPath(),24,80}, error), qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains('>'),10000);
        QElapsedTimer timer; timer.start(); process.stop(); QVERIFY(timer.elapsed() < 5000); process.stop();
    }
    void fullScreenEditor() {
        auto vim = QStandardPaths::findExecutable("vim");
#ifdef Q_OS_WIN
        if (vim.isEmpty() && QFileInfo::exists("C:/Program Files/Git/usr/bin/vim.exe")) vim = "C:/Program Files/Git/usr/bin/vim.exe";
#endif
        if (vim.isEmpty()) QSKIP("Vim is not installed");
        QTemporaryDir temp; Process process; Screen screen(24,80); QString error;
        connect(&process,&Process::received,&screen,&Screen::feed); connect(&screen,&Screen::output,&process,&Process::write);
        QSignalSpy exit(&process,&Process::exited);
        QVERIFY2(process.start({vim,{"-Nu","NONE","-n","-i","NONE","proof.txt"},temp.path(),24,80},error),qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(screen.alternate(),10000);
        process.write("iVim through Terlox\033:wq\r");
        QTRY_COMPARE_WITH_TIMEOUT(exit.size(),1,10000);
        QFile file(temp.filePath("proof.txt")); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll().trimmed(),QByteArray("Vim through Terlox"));
    }
    void interruptExternalProgram() {
        Process process; Screen screen(24,100); QString error;
        connect(&process,&Process::received,&screen,&Screen::feed); connect(&screen,&Screen::output,&process,&Process::write);
        QVERIFY2(process.start({shell(),{},QDir::tempPath(),24,100},error),qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains('>'),10000);
        process.write(("\"" + QCoreApplication::applicationFilePath() + "\" --wait-child\r").toUtf8());
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains("child-running\n"),10000);
        process.write(QByteArray(1,char(3)));
        process.write("echo interrupt-returned\r");
        QTRY_VERIFY_WITH_TIMEOUT(content(screen).contains("interrupt-returned\n"),10000);
    }
};
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    if (app.arguments().contains("--palette-query-child")) return runTerminalClientFixture();
    if (app.arguments().contains("--hidden-input-child")) return runHiddenInputFixture();
    if (app.arguments().contains("--tty-child")) {
#ifdef Q_OS_WIN
        const auto tty=[](FILE* f) { return _isatty(_fileno(f))?1:0; };
#else
        const auto tty=[](FILE* f) { return isatty(fileno(f))?1:0; };
#endif
        std::cout << "tty-in=" << tty(stdin) << " tty-out=" << tty(stdout) << " tty-err=" << tty(stderr) << '\n' << std::flush;
        return 0;
    }
    if (app.arguments().contains("--protocol-child")) {
        QImage image(1,1,QImage::Format_RGB32); image.fill(QColor("#12abef")); QByteArray png; QBuffer buffer(&png); buffer.open(QIODevice::WriteOnly); image.save(&buffer,"PNG");
        std::cout << "\033]777;terlox;" << QByteArray("{\"opacity\":0.4}").toBase64().constData() << "\a";
        std::cout << "\033]1337;File=inline=1;width=1;height=1:" << png.toBase64().constData() << "\a";
        std::cout << "\033[38;2;255;0;0mR\033[38;2;0;255;0mG";
        std::cout << "\033]777;terlox;" << QByteArray("{\"textOpacity\":0.25}").toBase64().constData() << "\aA\033[0m" << std::flush; return 0;
    }
    if (app.arguments().contains("--environment-child")) { std::cout << qEnvironmentVariable("TERLOX_ENV_PROBE").toStdString() << "\n" << std::flush; return 0; }
    if (app.arguments().contains("--wait-child")) { std::cout << "child-running\n" << std::flush; std::this_thread::sleep_for(std::chrono::seconds(30)); return 0; }
    QTemporaryDir historyDirectory;
    if (!historyDirectory.isValid()) return 1;
    qputenv("TERLOX_HISTORY_FILE",historyDirectory.filePath("history").toUtf8());
    ProcessTests tests; return QTest::qExec(&tests,argc,argv);
}
#include "ProcessTests.moc"
