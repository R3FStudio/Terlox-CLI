#include "installer/wizard/SetupWizard.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QAbstractButton>
#include <QDir>
#include <QFontDatabase>
#include <QCheckBox>
using namespace terlox::setup;
class SetupWizardTests:public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
#ifdef Q_OS_WIN
        // The offscreen Windows plugin has no system font enumeration.
        QVERIFY(QFontDatabase::addApplicationFont(qEnvironmentVariable("SystemRoot","C:/Windows")+"/Fonts/arial.ttf")>=0);
#endif
    }
    void reviewAndInstall() {
        QTemporaryDir root; QVERIFY(root.isValid()); const auto source=root.filePath("package"),target=root.filePath("installed"); QVERIFY(QDir().mkpath(source));
#ifdef Q_OS_WIN
        const auto platform="windows";
#elif defined(Q_OS_MACOS)
        const auto platform="macos";
#else
        const auto platform="linux";
#endif
        QFile marker(source+"/.terlox-package.json"); QVERIFY(marker.open(QIODevice::WriteOnly)); marker.write(QJsonDocument(QJsonObject{{"application","app.terlox.terminal"},{"platform",platform},{"version","test"},{"executable","terminal"}}).toJson()); marker.close();
        QFile terminal(source+"/terminal"); QVERIFY(terminal.open(QIODevice::WriteOnly)); terminal.write("fixture"); terminal.close();
        SetupWizard wizard(source,nullptr,false); wizard.setField("destination",target); wizard.setField("integration",false); wizard.show();
        QCOMPARE(wizard.currentId(),int(SetupWizard::Welcome)); wizard.next(); QCOMPARE(wizard.currentId(),int(SetupWizard::Destination));
        wizard.next(); QCOMPARE(wizard.currentId(),int(SetupWizard::Integration)); wizard.next(); QCOMPARE(wizard.currentId(),int(SetupWizard::Review));
        QVERIFY(wizard.button(QWizard::CommitButton)->isEnabled());
        if (!qEnvironmentVariableIsEmpty("TERLOX_SETUP_SCREENSHOT")) { QTest::qWait(100); QVERIFY(wizard.grab().save(qEnvironmentVariable("TERLOX_SETUP_SCREENSHOT"))); }
        wizard.next(); QCOMPARE(wizard.currentId(),int(SetupWizard::Install));
        QTRY_COMPARE_WITH_TIMEOUT(wizard.currentId(),int(SetupWizard::Finish),15000);
        QVERIFY2(wizard.succeeded(),qPrintable(wizard.resultText())); QVERIFY(QFileInfo::exists(target+"/.terlox-install.json"));
        QVERIFY(!wizard.button(QWizard::BackButton)->isVisible()); wizard.setField("launch",false); wizard.accept();
    }
    void suggestedDestinationDoesNotRequireEditing() {
        SetupWizard wizard(QDir::tempPath()); wizard.show(); wizard.next();
        QCOMPARE(wizard.currentId(),int(SetupWizard::Destination));
        QCOMPARE(wizard.field("destination").toString(),defaultDestination());
        QVERIFY(!wizard.field("destination").toString().isEmpty());
        QVERIFY(wizard.button(QWizard::NextButton)->isEnabled());
        wizard.setField("destination",QString("   "));
        QVERIFY(!wizard.button(QWizard::NextButton)->isEnabled());
        wizard.setField("destination",defaultDestination());
        QVERIFY(wizard.button(QWizard::NextButton)->isEnabled());
    }
    void defaultTerminalChoiceFollowsPlatformSupport() {
        SetupWizard wizard(QDir::tempPath());
        auto* checkbox=wizard.findChild<QCheckBox*>("defaultTerminal"); QVERIFY(checkbox);
        QCOMPARE(checkbox->isEnabled(),canSelectDefaultTerminal());
#ifdef Q_OS_WIN
        if (QSysInfo::kernelVersion().section('.',2,2).toInt()>=22000) QVERIFY(checkbox->isEnabled());
#endif
        QVERIFY(!checkbox->isChecked());
        wizard.setField("integration",false); QVERIFY(!checkbox->isEnabled());
        wizard.setField("integration",true); QCOMPARE(checkbox->isEnabled(),canSelectDefaultTerminal());
    }
};
QTEST_MAIN(SetupWizardTests)
#include "SetupWizardTests.moc"
