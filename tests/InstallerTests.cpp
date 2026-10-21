#include "installer/model/InstallPlan.h"
#include "installer/backend/InstallEngine.h"
#include "installer/integration/DesktopIntegration.h"
#include "installer/uninstall/UninstallPlan.h"
#include <QProcess>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>
#include <QtTest>
using namespace terlox::setup;
class InstallerTests:public QObject {
    Q_OBJECT
    static void put(const QString& path,const QByteArray& bytes) {
        QDir().mkpath(QFileInfo(path).absolutePath()); QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(bytes),bytes.size());
    }
    static QByteArray read(const QString& path) { QFile file(path); if (!file.open(QIODevice::ReadOnly)) return {}; return file.readAll(); }
    static void payload(const QString& path) {
#ifdef Q_OS_WIN
        const QString platform="windows";
#elif defined(Q_OS_MACOS)
        const QString platform="macos";
#else
        const QString platform="linux";
#endif
        put(path+"/.terlox-package.json",QJsonDocument(QJsonObject{{"application","app.terlox.terminal"},{"platform",platform},{"version","test"},{"executable","terminal"}}).toJson());
        put(path+"/terminal","terminal-v1"); put(path+"/runtime/library","runtime-data");
    }
private slots:
#ifdef Q_OS_WIN
    void defaultFolderCanBeInstalledWithoutExistingParents() {
        QTemporaryDir root; QVERIFY(root.isValid()); const auto source=root.filePath("package"); payload(source);
        const auto previous=qgetenv("LOCALAPPDATA");
        const auto restore=qScopeGuard([&] { if(previous.isNull()) qunsetenv("LOCALAPPDATA"); else qputenv("LOCALAPPDATA",previous); });
        qputenv("LOCALAPPDATA",root.filePath("new profile").toUtf8());
        InstallPlan plan; QString error;
        const auto destination=defaultDestination();
        QVERIFY2(buildInstallPlan({source,destination,false,false,false},plan,error),qPrintable(error));
        const auto installed=installPackage(plan); QVERIFY2(installed.success,qPrintable(installed.message));
        QCOMPARE(read(destination+"/terminal"),QByteArray("terminal-v1"));
    }
#endif
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    void xdgPreferenceAndAbsoluteLauncher() {
        QTemporaryDir root; QVERIFY(root.isValid());
        const auto oldConfig=qgetenv("XDG_CONFIG_HOME"),oldData=qgetenv("XDG_DATA_HOME"),oldDesktop=qgetenv("XDG_CURRENT_DESKTOP");
        const auto restore=qScopeGuard([&] {
            for (const auto& item:{qMakePair("XDG_CONFIG_HOME",oldConfig),qMakePair("XDG_DATA_HOME",oldData),qMakePair("XDG_CURRENT_DESKTOP",oldDesktop)}) {
                if (item.second.isNull()) qunsetenv(item.first); else qputenv(item.first,item.second);
            }
        });
        qputenv("XDG_CONFIG_HOME",root.filePath("config").toUtf8()); qputenv("XDG_DATA_HOME",root.filePath("data").toUtf8()); qputenv("XDG_CURRENT_DESKTOP","GNOME:../../ignored");
        put(root.filePath("config/xdg-terminals.list"),"other.desktop\n# keep preference\n");
        InstallPlan plan; plan.options={root.filePath("package"),root.filePath("installed with spaces"),true,false,true}; plan.executable="bin/terlox";
        put(plan.options.destination+"/bin/terlox","#!/bin/sh\nexit 0\n");
        QVERIFY(QFile::setPermissions(plan.options.destination+"/bin/terlox",QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner));
        QString error; QStringList notes;
        QVERIFY2(registerDesktop(plan,error,notes),qPrintable(error));
        const auto first=read(root.filePath("config/xdg-terminals.list"));
        QVERIFY(first.startsWith("app.terlox.terminal.desktop\n")); QVERIFY(first.contains("other.desktop"));
        QVERIFY(registerDesktop(plan,error,notes)); QCOMPARE(read(root.filePath("config/xdg-terminals.list")),first);
        QVERIFY(QFileInfo::exists(root.filePath("config/gnome-xdg-terminals.list")));
        const auto launcher=read(root.filePath("data/applications/app.terlox.terminal.desktop"));
        QVERIFY(launcher.contains(("Exec=\""+plan.options.destination+"/bin/terlox\"").toUtf8()));
    }
#endif
    void rejectsRecursiveAndUnknownPackages() {
        QTemporaryDir root; const auto source=root.filePath("package"); payload(source); InstallPlan plan; QString error;
        QVERIFY(!buildInstallPlan({source,source+"/installed",false,false,false},plan,error));
        QVERIFY(!buildInstallPlan({source,root.path(),false,false,false},plan,error));
        QVERIFY(!buildInstallPlan({root.filePath("unknown"),root.filePath("install"),false,false,false},plan,error));
    }
    void installsAndUpgradesOnlyOwnedFiles() {
        QTemporaryDir root; const auto source=root.filePath("package"),destination=root.filePath("install"); payload(source);
        InstallPlan plan; QString error; QVERIFY2(buildInstallPlan({source,destination,false,false,false},plan,error),qPrintable(error));
        int progress=0; const auto result=installPackage(plan,[&](int value,const QString&) { progress=value; });
        QVERIFY2(result.success,qPrintable(result.message)); QCOMPARE(progress,100);
        QCOMPARE(read(destination+"/terminal"),QByteArray("terminal-v1"));
        QVERIFY(QFileInfo::exists(destination+"/.terlox-install.json"));
        put(destination+"/personal-note","keep-me"); put(source+"/terminal","terminal-v2");
        const auto upgrade=installPackage(plan); QVERIFY2(upgrade.success,qPrintable(upgrade.message));
        QCOMPARE(read(destination+"/terminal"),QByteArray("terminal-v2")); QCOMPARE(read(destination+"/personal-note"),QByteArray("keep-me"));
    }
    void refusesUnrelatedFileCollision() {
        QTemporaryDir root; const auto source=root.filePath("package"),destination=root.filePath("install"); payload(source); put(destination+"/terminal","unrelated");
        InstallPlan plan; QString error; QVERIFY(!buildInstallPlan({source,destination,false,false,false},plan,error));
        QCOMPARE(read(destination+"/terminal"),QByteArray("unrelated"));
    }
    void uninstallKeepsUserFilesAndRemovesOnlyInventory() {
        QTemporaryDir root; const auto source=root.filePath("package"),destination=root.filePath("installed"); payload(source);
        put(source+"/old-runtime.dat","old-version");
        InstallPlan plan; QString error; QStringList notes;
        QVERIFY(buildInstallPlan({source,destination,false,false,false},plan,error)); QVERIFY(installPackage(plan).success);
        QVERIFY(QFile::remove(source+"/old-runtime.dat")); QVERIFY(installPackage(plan).success);
        put(destination+"/my-work/keep.txt","personal-data");
        QVERIFY2(uninstallPackage(destination,error,notes,false),qPrintable(error));
        QVERIFY(!QFileInfo::exists(destination+"/terminal")); QVERIFY(!QFileInfo::exists(destination+"/runtime"));
        QVERIFY(!QFileInfo::exists(destination+"/.terlox-install.json"));
        QVERIFY(!QFileInfo::exists(destination+"/old-runtime.dat"));
        QCOMPARE(read(destination+"/my-work/keep.txt"),QByteArray("personal-data"));
    }
    void uninstallRejectsEscapingInventoryBeforeDeletingAnything() {
        QTemporaryDir root; const auto directory=root.filePath("installed"); QDir().mkpath(directory);
        put(root.filePath("outside.txt"),"keep"); put(directory+"/owned.txt","keep-on-invalid-plan");
        QStringList paths{"../outside.txt","nested/../../outside.txt",root.filePath("outside.txt"),"owned.txt:stream"};
#ifdef Q_OS_WIN
        paths << ".. /outside.txt";
#endif
        for (const auto& unsafe:paths) {
            put(directory+"/.terlox-install.json",QJsonDocument(QJsonObject{{"application","app.terlox.terminal"},{"files",QJsonArray{"owned.txt",unsafe}}}).toJson());
            QString error; QStringList notes; QVERIFY(!uninstallPackage(directory,error,notes,false));
            QCOMPARE(read(root.filePath("outside.txt")),QByteArray("keep")); QCOMPARE(read(directory+"/owned.txt"),QByteArray("keep-on-invalid-plan"));
        }
    }
#ifdef Q_OS_WIN
    void uninstallRefusesRunningSessionWithoutStoppingIt() {
        QTemporaryDir root; const auto directory=root.filePath("installed"); QDir().mkpath(directory);
        const auto running=directory+"/busy.exe";
        QVERIFY(QFile::copy(QCoreApplication::applicationDirPath()+"/terloxsh.exe",running));
        put(directory+"/.terlox-install.json",QJsonDocument(QJsonObject{{"application","app.terlox.terminal"},{"files",QJsonArray{"busy.exe"}}}).toJson());
        QProcess session; session.start(running,{}); QVERIFY(session.waitForStarted());
        const auto stop=qScopeGuard([&] { session.kill(); session.waitForFinished(); });
        QString error; QStringList notes; QVERIFY(!uninstallPackage(directory,error,notes,false));
        QVERIFY2(error.contains("Close"),qPrintable(error)); QCOMPARE(session.state(),QProcess::Running); QVERIFY(QFileInfo::exists(running));
    }
#endif
    void rollsBackPartialUpgrade() {
        QTemporaryDir root; const auto source=root.filePath("package"),destination=root.filePath("install"); payload(source);
        InstallPlan plan; QString error; QVERIFY(buildInstallPlan({source,destination,false,false,false},plan,error)); QVERIFY(installPackage(plan).success);
        put(source+"/terminal","terminal-v2"); put(source+"/z-blocked/file","new-data"); put(destination+"/z-blocked","unrelated-file");
        const auto result=installPackage(plan); QVERIFY(!result.success);
        QCOMPARE(read(destination+"/terminal"),QByteArray("terminal-v1")); QCOMPARE(read(destination+"/z-blocked"),QByteArray("unrelated-file"));
    }
    void unsupportedDefaultCannotBeForced() {
        if (canSelectDefaultTerminal()) QSKIP("This platform supports the XDG default option");
        QTemporaryDir root; const auto source=root.filePath("package"); payload(source); InstallPlan plan; QString error;
        QVERIFY(!buildInstallPlan({source,root.filePath("install"),true,false,true},plan,error));
        QVERIFY(error.contains("unavailable"));
    }
};
QTEST_GUILESS_MAIN(InstallerTests)
#include "InstallerTests.moc"
