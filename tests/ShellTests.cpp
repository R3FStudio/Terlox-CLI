#include "shell/parsing/Parser.h"
#include "shell/editing/History.h"
#include "terminal/Screen.h"
#include "platform/Elevation.h"
#include "shell/runtime/NativeShell.h"
#include <QtTest>
class CoreTests : public QObject {
    Q_OBJECT
private slots:
#ifdef Q_OS_WIN
    void elevationRequestPreservesCommand() {
        const QString command="ps Write-Output 'hello \"world\"'; Get-Location";
        const auto request=terlox::elevatedTerminalRequest("C:/Program Files/Terlox",command,"C:/directory with spaces/");
        QCOMPARE(request.executable,QString("C:/Program Files/Terlox/terlox.exe"));
        QCOMPARE(request.arguments[1],request.directory);
        QCOMPARE(QString::fromUtf8(QByteArray::fromBase64(request.arguments[3].toLatin1())),command);
        const auto interactive=terlox::elevatedTerminalRequest("C:/Terlox","","C:/"); QCOMPARE(interactive.arguments.size(),4);
        const auto program=terlox::elevatedProgramRequest("C:/Terlox","C:/program.exe",{"path with spaces","a&b","--profile"},"C:/folder");
        QCOMPARE(program.arguments,QStringList({"--directory","C:/folder","-e","C:/program.exe","path with spaces","a&b","--profile"}));
        QString error; QVERIFY(!terlox::elevateTerminal({"Z:/missing-terlox.exe",{},"C:/"},error)); QVERIFY(!error.isEmpty());
    }
    void nativeAdminManifest() {
        const auto diskpart=qEnvironmentVariable("SystemRoot","C:/Windows")+"/System32/diskpart.exe";
        QVERIFY(terlox::executableRequiresElevation(diskpart));
        QVERIFY(!terlox::executableRequiresElevation(QCoreApplication::applicationFilePath()));
        QCOMPARE(terlox::parseAdminCommand("--cmd diskpart").engine,QString("cmd"));
        QCOMPARE(terlox::parseAdminCommand("ps Write-Output 'a;b'").command,QString("Write-Output 'a;b'"));
    }
    void automaticElevationUsesNativeConsent() {
        if (terlox::isAdministrator()) QSKIP("Requires an ordinary unelevated process");
        const auto command='"'+qEnvironmentVariable("SystemRoot","C:/Windows")+"/System32/diskpart.exe\"";
        int requests=0; terlox::ElevationRequest captured;
        auto launch=[&](const terlox::ElevationRequest& request,QString&) { ++requests; captured=request; return true; };
        const auto result=terlox::shell::nativeShellLine(command,launch);
        QVERIFY(result.has_value()); QCOMPARE(*result,0); QCOMPARE(requests,1);
        QCOMPARE(captured.arguments.mid(0,3),QStringList({"--directory",QDir::currentPath(),"-e"}));
        QCOMPARE(QFileInfo(captured.arguments[3]).fileName(),QString("diskpart.exe"));
        auto cancel=[&](const terlox::ElevationRequest&,QString& error) { ++requests; error="UAC request cancelled; no command was run."; return false; };
        QCOMPARE(*terlox::shell::nativeShellLine(command,cancel),1); QCOMPARE(requests,2);
        QVERIFY(!terlox::shell::nativeShellLine("git --version",launch).has_value()); QCOMPARE(requests,2);
    }
    void windowsSudoRequestsUac() {
        int requests=0; terlox::ElevationRequest captured;
        auto launch=[&](const terlox::ElevationRequest& request,QString&) { ++requests; captured=request; return true; };
        QCOMPARE(*terlox::shell::nativeShellLine("sudo --help",launch),0); QCOMPARE(requests,0);
        QCOMPARE(*terlox::shell::nativeShellLine("sudo --cmd echo privileged-command",launch),0);
        QCOMPARE(requests,1); QCOMPARE(captured.arguments.last(),QString("cmd"));
        QCOMPARE(QString::fromUtf8(QByteArray::fromBase64(captured.arguments[3].toLatin1())),QString("echo privileged-command"));
        QCOMPARE(*terlox::shell::nativeShellLine("sudo -i",launch),0); QCOMPARE(requests,2);
        QVERIFY(!captured.arguments.contains("--admin-command"));
        QCOMPARE(*terlox::shell::nativeShellLine("sudo -u other command",launch),2); QCOMPARE(requests,2);
    }
    void windowsPaths() {
        for (const auto& line:QStringList{"cd C:\\", "cd \"C:\\\"", "cd \"C:\\directory with spaces\\\"", "cd \\\\server\\share\\"}) {
            const auto p=terlox::shell::parse(line); QVERIFY2(p.error.isEmpty(),qPrintable(p.error));
            const auto path=p.program[0][0].arguments[1];
            QVERIFY(path.endsWith('\\')); QVERIFY(!path.contains('"'));
            if (line.contains("server")) QVERIFY(path.startsWith("\\\\server"));
        }
    }
#endif
    void parsing() { auto p = terlox::shell::parse("echo \"two words\" '' a\\ b | other > output"); QVERIFY(p.error.isEmpty()); QCOMPARE(p.program[0].size(), size_t(2)); QCOMPARE(p.program[0][0].arguments, QStringList({"echo","two words","","a b"})); QCOMPARE(p.program[0][1].output, "output"); }
    void malformed() { QVERIFY(!terlox::shell::parse("echo '").error.isEmpty()); QVERIFY(!terlox::shell::parse("echo |").error.isEmpty()); QVERIFY(!terlox::shell::parse("echo >").error.isEmpty()); }
    void expansion() { qputenv("TERLOX_TEST", "works"); auto p = terlox::shell::parse("echo '$TERLOX_TEST' \"${TERLOX_TEST}\" $?", 7); QCOMPARE(p.program[0][0].arguments, QStringList({"echo","$TERLOX_TEST","works","7"})); }
    void history() { terlox::shell::History h; h.add("one"); h.add("two"); h.add("two"); QCOMPARE(h.entries().size(), 2); QCOMPARE(h.previous("draft"), "two"); QCOMPARE(h.previous(""), "one"); QCOMPARE(h.next(), "two"); QCOMPARE(h.next(), "draft"); }
    void screen() { terlox::Screen s(3, 10); s.feed("hello\033[2;3HX"); QCOMPARE(s.text(0,0,4), "hello"); QCOMPARE(s.text(1,2,2), "X"); s.feed("\033[2J"); QCOMPARE(s.text(0,0,9), ""); }
    void alternate() { terlox::Screen s(3,10); s.feed("main\033[?1049h\033[Hother"); QVERIFY(s.alternate()); s.feed("\033[?1049l"); QVERIFY(!s.alternate()); QCOMPARE(s.text(0,0,3), "main"); }
    void unicode() { terlox::Screen s(3,10); s.feed(QString::fromUtf8("e\u0301界").toUtf8()); QCOMPARE(s.cell(0,0).chars[1], uint32_t(0x301)); QCOMPARE(s.cell(0,1).width, char(2)); }
    void trueColorAndVisibility() { terlox::Screen s(3,10); s.feed("X"); QCOMPARE(s.color(s.cell(0,0).fg),QColor(Qt::white)); s.feed("\r\033[38;2;20;40;60mX\033[?25l"); QCOMPARE(s.color(s.cell(0,0).fg),QColor(20,40,60)); QVERIFY(!s.cursorVisible()); s.feed("\033[?25h"); QVERIFY(s.cursorVisible()); }
    void scrollAndResize() { terlox::Screen s(2,10,3); for (int i=0;i<8;++i) s.feed("line\r\n"); QCOMPARE(s.historySize(),3); QCOMPARE(s.text(0,0,3,1),"line"); s.resize(4,20); QCOMPARE(s.rows(),4); QCOMPARE(s.columns(),20); }
    void fragmentedUtf8AndEscapes() { terlox::Screen s(3,10); s.feed(QByteArray::fromHex("e7")); s.feed(QByteArray::fromHex("958c")); QCOMPARE(s.cell(0,0).width,char(2)); s.feed("\033["); s.feed("2;4H!"); QCOMPARE(s.text(1,3,3),"!"); }
};
QTEST_GUILESS_MAIN(CoreTests)
#include "ShellTests.moc"
