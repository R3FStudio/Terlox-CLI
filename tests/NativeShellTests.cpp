#include <QtTest>
#include <QCoreApplication>
#include <QProcess>
#include <QTemporaryDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QDir>
#include <QFileInfo>
#include "information/Version.h"
#include "platform/Elevation.h"
class NativeShellTests : public QObject {
    Q_OBJECT
    QString shell() const {
#ifdef Q_OS_WIN
        return QCoreApplication::applicationDirPath()+"/terloxsh.exe";
#else
        return QCoreApplication::applicationDirPath()+"/terloxsh";
#endif
    }
private slots:
    void commandAndStay() {
        QProcess p; p.start(shell(),{"--command-and-stay","echo before-elevation-session"}); QVERIFY(p.waitForStarted());
        p.write("echo after-elevation-session\nexit 19\n"); p.closeWriteChannel();
        QVERIFY(p.waitForFinished(10000)); QCOMPARE(p.exitCode(),19);
        const auto output=p.readAllStandardOutput(); QVERIFY(output.contains("before-elevation-session")); QVERIFY(output.contains("after-elevation-session"));
    }
    void information() {
        QProcess p; p.start(shell(),{"-c","/info-terlox"}); QVERIFY(p.waitForFinished(10000)); QCOMPARE(p.exitCode(),0);
        const auto text=p.readAllStandardOutput(); QVERIFY(text.contains(TERLOX_VERSION)); QVERIFY(text.contains("KEYBOARD SHORTCUTS")); QVERIFY(text.contains("WINDOWS")); QVERIFY(text.contains("LINUX")); QVERIFY(text.contains("MACOS")); QVERIFY(text.contains("0.1.0"));
    }
#ifdef Q_OS_WIN
    void sudoHelpDoesNotRequestUac() {
        QProcess p; p.start(shell(),{"-c","sudo --help"}); QVERIFY(p.waitForFinished(10000));
        QCOMPARE(p.exitCode(),0); QVERIFY(p.readAllStandardOutput().contains("Windows UAC"));
    }
    void nativeCommands_data() {
        QTest::addColumn<QString>("command"); QTest::addColumn<QByteArray>("expected");
        QTest::newRow("powershell-code") << QString("ps Write-Output ('power' + 'shell-ok')") << QByteArray("powershell-ok");
        QTest::newRow("powershell-pipeline") << QString("ps 'native','pipeline' | ForEach-Object { $_.ToUpper() }") << QByteArray("PIPELINE");
        QTest::newRow("direct-cmdlet") << QString("Get-Date -Format 'yyyy-MM-dd'") << QDate::currentDate().toString("yyyy-MM-dd").toLatin1();
        QTest::newRow("cmd-code") << QString("cmd echo cmd-ok") << QByteArray("cmd-ok");
        QTest::newRow("cmd-builtin") << QString("ver") << QByteArray("Microsoft Windows");
        QTest::newRow("cmd-builtin-outside-alias-list") << QString("if 1==1 echo native-if-ok") << QByteArray("native-if-ok");
        QTest::newRow("cmd-loop-outside-alias-list") << QString("for %T in (native-for-ok) do @echo %T") << QByteArray("native-for-ok");
        QTest::newRow("windows-environment-variable") << QString("echo %SystemRoot%") << qEnvironmentVariable("SystemRoot").toUtf8();
        QTest::newRow("powershell-ls-alias") << QString("ls \""+shell()+"\" | Select-Object -ExpandProperty Name") << QByteArray("terloxsh.exe");
        QTest::newRow("uppercase-cd") << QString("CD /d \"C:\\\"; pwd") << QByteArray("C:/");
        QTest::newRow("location-cmdlet-persists") << QString("Set-Location \"C:\\\"; pwd") << QByteArray("C:/");
        QTest::newRow("com-program") << QString("more /?") << QByteArray("MORE");
        QTest::newRow("admin-help-no-uac") << QString("admin --help") << QByteArray("native elevated shell");
    }
    void nativeCommands() {
        QFETCH(QString,command); QFETCH(QByteArray,expected);
        QProcess p;
        if (QByteArray(QTest::currentDataTag())=="powershell-ls-alias") {
            // Test the alias fallback without an external ls on the runner's PATH.
            auto environment=QProcessEnvironment::systemEnvironment();
            QStringList paths;
            for (auto path:environment.value("PATH").split(';')) {
                if (path.startsWith('"') && path.endsWith('"')) path=path.mid(1,path.size()-2);
                const QDir directory(path); bool providesAlias=false;
                for (const auto* suffix:{".exe",".com",".cmd",".bat",""})
                    if (QFileInfo::exists(directory.filePath(QString("ls")+suffix))) providesAlias=true;
                if (!providesAlias) paths << path;
            }
            environment.insert("PATH",paths.join(';'));
            p.setProcessEnvironment(environment);
        }
        p.start(shell(),{"-c",command}); QVERIFY(p.waitForFinished(15000));
        QVERIFY2(p.exitCode()==0,p.readAllStandardError().constData()); QVERIFY(p.readAllStandardOutput().contains(expected));
    }
    void scripts_data() {
        QTest::addColumn<QString>("extension"); QTest::addColumn<QByteArray>("source"); QTest::addColumn<int>("status");
        QTest::newRow("powershell-file") << QString("ps1") << QByteArray("param([string]$Name)\nWrite-Output \"script-$Name\"\nexit 9\n") << 9;
        QTest::newRow("cmd-file") << QString("cmd") << QByteArray("@echo off\r\necho script-%~1\r\nexit /b 6\r\n") << 6;
        QTest::newRow("bat-file") << QString("bat") << QByteArray("@echo off\r\necho script-%~1\r\nexit /b 6\r\n") << 6;
    }
    void persistentDirectoryCommands() {
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        QVERIFY(QDir(temporary.path()).mkdir("directory with spaces"));
        const auto target=temporary.filePath("directory with spaces");
        const auto root=QDir::rootPath();
        QProcess p;
        auto environment=QProcessEnvironment::systemEnvironment(); environment.insert("TERLOX_CD_TARGET",target); p.setProcessEnvironment(environment);
        p.start(shell(),{}); QVERIFY(p.waitForStarted());
        p.write(("cd /d \""+QDir::toNativeSeparators(target)+"\\\"\npwd\ncd ..\npwd\ncd %TERLOX_CD_TARGET%\npwd\ncd "+root+"\npwd\nexit\n").toUtf8());
        p.closeWriteChannel(); QVERIFY(p.waitForFinished(15000)); QCOMPARE(p.exitCode(),0);
        const auto lines=QString::fromUtf8(p.readAllStandardOutput()).replace("\r","").split('\n');
        QCOMPARE(lines.count(QDir::fromNativeSeparators(target)),2);
        QVERIFY(lines.contains(QDir::fromNativeSeparators(temporary.path())));
        QVERIFY(lines.contains(root));
    }
    void scripts() {
        QFETCH(QString,extension); QFETCH(QByteArray,source); QFETCH(int,status);
        QTemporaryDir temporary; const auto path=temporary.filePath("script with spaces."+extension);
        QFile f(path); QVERIFY(f.open(QIODevice::WriteOnly)); f.write(source); f.close();
        QProcess p; p.start(shell(),{"-c","\""+path+"\" \"two words\""}); QVERIFY(p.waitForFinished(15000));
        QVERIFY2(p.exitCode()==status,p.readAllStandardError().constData()); QVERIFY(p.readAllStandardOutput().contains("script-two words"));
    }
#endif
};
QTEST_GUILESS_MAIN(NativeShellTests)
#include "NativeShellTests.moc"
