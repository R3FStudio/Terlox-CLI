#include "integration/defaults/DefaultTerminal.h"
#include "integration/defaults/Command.h"
#include <QtTest>
#include <QFile>
#include <QTemporaryDir>
#ifdef Q_OS_WIN
#include "integration/windows/HandoffIds.h"
#include <QLibrary>
#include <windows.h>
#include <objbase.h>
#endif
using namespace terlox;
class DefaultTerminalTests:public QObject {
    Q_OBJECT
private slots:
    void invalidCommandDoesNotChangePreferences() {
        const auto before=defaultTerminalStatus().message;
        QCOMPARE(defaultTerminalCommand({"unknown"},"missing"),2);
        QCOMPARE(defaultTerminalStatus().message,before);
    }
    void missingInstallationDoesNotChangePreferences() {
        QTemporaryDir temporary;
        const auto before=defaultTerminalStatus().message;
        QVERIFY(!selectDefaultTerminal(temporary.filePath("missing")).success);
        QCOMPARE(defaultTerminalStatus().message,before);
    }
#ifdef Q_OS_WIN
    void proxyExportsItsRegisteredClass() {
        QLibrary proxy(QCoreApplication::applicationDirPath()+"/terlox-handoff-" TERLOX_TEST_VERSION ".dll");
        using GetClass=HRESULT (WINAPI*)(REFCLSID,REFIID,void**);
        const auto get=reinterpret_cast<GetClass>(proxy.resolve("DllGetClassObject")); QVERIFY2(get,qPrintable(proxy.errorString()));
        CLSID clsid; QVERIFY(SUCCEEDED(CLSIDFromString(QString(ProxyClass).toStdWString().c_str(),&clsid)));
        IID factory; QVERIFY(SUCCEEDED(IIDFromString(L"{D5F569D0-593B-101A-B569-08002B2DBF7A}",&factory)));
        IUnknown* value=nullptr; const auto result=get(clsid,factory,reinterpret_cast<void**>(&value));
        QVERIFY2(SUCCEEDED(result),qPrintable(QString::number(quint32(result),16))); QVERIFY(value); value->Release();
    }
#elif !defined(Q_OS_MACOS)
    void preferenceRoundTripPreservesOthers() {
        QTemporaryDir temporary; const auto oldConfig=qgetenv("XDG_CONFIG_HOME"),oldData=qgetenv("XDG_DATA_HOME"),oldDesktop=qgetenv("XDG_CURRENT_DESKTOP"),oldImage=qgetenv("APPIMAGE");
        qputenv("XDG_CONFIG_HOME",temporary.path().toUtf8()); qputenv("XDG_DATA_HOME",(temporary.path()+"/data").toUtf8()); qputenv("XDG_CURRENT_DESKTOP","GNOME:invalid/path"); qunsetenv("APPIMAGE");
        const auto preferences=temporary.filePath("xdg-terminals.list");
        QFile file(preferences); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("# keep this\nother.desktop\n"); file.close();
        const auto executable=QCoreApplication::applicationFilePath();
        QVERIFY(selectDefaultTerminal(executable).success); QVERIFY(selectDefaultTerminal(executable).success);
        QVERIFY(file.open(QIODevice::ReadOnly)); const auto selected=file.readAll(); file.close();
        QCOMPARE(selected,QByteArray("app.terlox.terminal.desktop\n# keep this\nother.desktop\n"));
        QVERIFY(QFileInfo::exists(temporary.filePath("gnome-xdg-terminals.list")));
        qputenv("XDG_CURRENT_DESKTOP","KDE");
        QVERIFY(restoreDefaultTerminal().success); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(),QByteArray("# keep this\nother.desktop\n")); file.close();
        QVERIFY(!QFileInfo::exists(temporary.filePath("gnome-xdg-terminals.list")));
        qputenv("XDG_CONFIG_HOME",oldConfig); qputenv("XDG_DATA_HOME",oldData); qputenv("XDG_CURRENT_DESKTOP",oldDesktop); qputenv("APPIMAGE",oldImage);
    }
#endif
};
QTEST_GUILESS_MAIN(DefaultTerminalTests)
#include "DefaultTerminalTests.moc"
