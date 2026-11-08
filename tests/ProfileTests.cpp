#include "settings/Store.h"
#include <QtTest>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <algorithm>
using namespace terlox;
class ProfileTests : public QObject {
    Q_OBJECT
    QTemporaryDir config_;
private slots:
    void initTestCase() {
        QVERIFY(config_.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,config_.path());
        QCoreApplication::setOrganizationName("TerloxProfileTests");
        QCoreApplication::setApplicationName("TerloxProfileTests");
    }
    void cleanup() { QSettings().clear(); }
    void automaticDefaultAndExplicitPreference() {
#ifdef Q_OS_WIN
        QCOMPARE(Store().defaultProfile(),QString("Terlox shell"));
#else
        QVERIFY(Store().defaultProfile().isEmpty());
#endif
        Store().setDefaultProfile("Terlox shell");
        QCOMPARE(Store().defaultProfile(),QString("Terlox shell"));
        Store().setDefaultProfile("custom-shell");
        QCOMPARE(Store().defaultProfile(),QString("custom-shell"));
    }
#ifndef Q_OS_WIN
    void nativeProfilesComeBeforeIntegratedShell() {
        QTemporaryDir app; QVERIFY(app.isValid());
        QFile integrated(app.filePath("terloxsh")); QVERIFY(integrated.open(QIODevice::WriteOnly)); integrated.close();
        const auto profiles=discoverProfiles(app.path()); QVERIFY(!profiles.isEmpty());
        QCOMPARE(profiles.last().name,QString("Terlox shell"));
        QVERIFY(profiles.first().name!="Terlox shell");
        QVERIFY(profiles.first().launch.arguments.contains("-i"));
    }
    void preferredNativeShell() {
        const auto sh=QStandardPaths::findExecutable("sh"); QVERIFY(!sh.isEmpty());
        const auto previous=qgetenv("SHELL");
        const bool wasSet=qEnvironmentVariableIsSet("SHELL");
        const auto restore=qScopeGuard([&] { if (wasSet) qputenv("SHELL",previous); else qunsetenv("SHELL"); });
        qputenv("SHELL",sh.toUtf8());
        const auto profiles=discoverProfiles(config_.path()); QVERIFY(!profiles.isEmpty());
        QCOMPARE(QFileInfo(profiles.first().launch.executable).canonicalFilePath(),QFileInfo(sh).canonicalFilePath());
        qputenv("SHELL","/missing/shell");
        QVERIFY(!discoverProfiles(config_.path()).isEmpty());
    }
    void integratedFallbackWithoutNativeShell() {
        QTemporaryDir app; QVERIFY(app.isValid());
        QFile integrated(app.filePath("terloxsh")); QVERIFY(integrated.open(QIODevice::WriteOnly)); integrated.close();
        const auto previous=qgetenv("PATH");
        const bool wasSet=qEnvironmentVariableIsSet("PATH");
        const auto restore=qScopeGuard([&] { if (wasSet) qputenv("PATH",previous); else qunsetenv("PATH"); });
        qputenv("PATH",app.path().toUtf8());
        const auto profiles=discoverProfiles(app.path()); QCOMPARE(profiles.size(),1);
        QCOMPARE(profiles.first().name,QString("Terlox shell"));
    }
    void bashStartupAndFunctionsPersist() {
        const auto bash=QStandardPaths::findExecutable("bash");
        if (bash.isEmpty()) QSKIP("Bash is not installed");
        QTemporaryDir home; QVERIFY(home.isValid());
        auto write=[](const QString& path,const QByteArray& contents) {
            QFile file(path); return file.open(QIODevice::WriteOnly) && file.write(contents)==contents.size();
        };
        QVERIFY(QDir(home.path()).mkpath(".nvm/bin"));
        // A local fixture exercises the same state that nvm needs, without
        // downloading Node or touching the developer's real shell setup.
        QVERIFY(write(home.filePath(".nvm/nvm.sh"),
            "nvm() { export PATH=\"$HOME/.nvm/bin:$PATH\"; export TERLOX_NVM_STATE=loaded; }\n"));
        QVERIFY(write(home.filePath(".bashrc"),". \"$HOME/.nvm/nvm.sh\"\necho startup-loaded\n"));
        const auto node=home.filePath(".nvm/bin/node");
        QVERIFY(write(node,"#!/bin/sh\nprintf 'fixture-node-26\\n'\n"));
        QVERIFY(QFile::setPermissions(node,QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner));
        const auto profiles=discoverProfiles(config_.path());
        const auto it=std::find_if(profiles.begin(),profiles.end(),[](const Profile& p) { return p.name=="bash"; });
        QVERIFY(it!=profiles.end());
        auto environment=QProcessEnvironment::systemEnvironment();
        environment.insert("HOME",home.path()); environment.remove("BASH_ENV"); environment.remove("ENV");
        // Linux's interactive Bash reads .bashrc. On macOS its login profile
        // also needs to source .bashrc, just as a user's normal login does.
        QVERIFY(write(home.filePath(".bash_profile"),". \"$HOME/.bashrc\"\n"));
        QProcess process; process.setProcessEnvironment(environment);
        process.setWorkingDirectory(home.path());
        process.start(it->launch.executable,it->launch.arguments); QVERIFY(process.waitForStarted());
        process.write("\\. \"$HOME/.nvm/nvm.sh\"\nnvm install 26\nnode --version\nprintenv TERLOX_NVM_STATE\nexit\n");
        process.closeWriteChannel(); QVERIFY(process.waitForFinished(10000)); QCOMPARE(process.exitCode(),0);
        const auto output=process.readAllStandardOutput();
        QVERIFY2(output.contains("startup-loaded"),output.constData());
        QVERIFY2(output.contains("fixture-node-26"),output.constData());
        QVERIFY2(output.contains("loaded\n"),output.constData());
        QVERIFY(!process.readAllStandardError().contains("command not found"));
    }
#endif
};
QTEST_GUILESS_MAIN(ProfileTests)
#include "ProfileTests.moc"
