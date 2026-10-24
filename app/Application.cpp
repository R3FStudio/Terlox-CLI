#include "Application.h"
#include "branding/Brand.h"
#include "LaunchArguments.h"
#include "window/Window.h"
#include "information/Version.h"
#include "renderer/GraphicsBackend.h"
#include "platform/Elevation.h"
#include "platform/Environment.h"
#include "settings/AdminProfile.h"
#include "integration/defaults/Command.h"
#include "updates/Service.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#ifdef Q_OS_WIN
#include "integration/windows/HandoffServer.h"
#endif
#ifdef Q_OS_MACOS
#include "FileOpenHandler.h"
#endif
#include <QDir>
#include <QFileInfo>
#include <QApplication>
#include <QCommandLineParser>
#include <QSaveFile>
#include <QTimer>
#include <QQuickWindow>
#include <iostream>
#include <optional>
namespace terlox {
int runApplication(int argc, char** argv) {
    configureGraphics();
    importTerminalEnvironment();
    QApplication app(argc, argv); app.setApplicationName("Terlox"); app.setOrganizationName("Terlox"); app.setApplicationVersion(TERLOX_VERSION);
    app.setDesktopFileName("app.terlox.terminal");
    app.setWindowIcon(brandIcon());
    updates::Service updateService;
    QObject::connect(&updateService,&updates::Service::restartRequested,&app,&QCoreApplication::quit);
#ifdef Q_OS_MACOS
    FileOpenHandler fileOpen(&app); app.installEventFilter(&fileOpen);
#endif
#ifdef Q_OS_WIN
    if (app.arguments().contains("-Embedding",Qt::CaseInsensitive)) {
        bool connected=false;
        HandoffServer server([&connected](std::shared_ptr<InboundSession> incoming) {
            connected=true; Profile profile{"Console",{}}; profile.launch.inbound=std::move(incoming);
            auto* window=new Window({},nullptr,&profile); window->setAttribute(Qt::WA_DeleteOnClose); window->show();
        });
        QString error; if (!server.listen(error)) { std::cerr << error.toStdString() << '\n'; return 3; }
        QTimer::singleShot(30000,&app,[&] { if (!connected) app.exit(3); });
        return app.exec();
    }
#endif
    QCommandLineParser parser; parser.setApplicationDescription("Native terminal and shell"); parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({"update-result","Result report from the update installer","path"});
    QCommandLineOption profile("profile", "Launch the named shell profile", "name"); parser.addOption(profile);
    QCommandLineOption diagnostics("diagnostics","Initialize a real terminal/GPU frame, print diagnostics and exit"); parser.addOption(diagnostics);
    QCommandLineOption diagnosticsOutput("diagnostics-output","Write diagnostics to this UTF-8 file","path"); parser.addOption(diagnosticsOutput);
    QCommandLineOption directory({"directory","working-directory"},"Starting working directory","path"); parser.addOption(directory);
    QCommandLineOption resetTextSize("reset-text-size","Restore the saved terminal text size to 12 pt"); parser.addOption(resetTextSize);
    QCommandLineOption setDefault("set-default-terminal","Select Terlox as this account's terminal preference"); parser.addOption(setDefault);
    QCommandLineOption defaultStatus("default-terminal-status","Show this account's terminal preference"); parser.addOption(defaultStatus);
    QCommandLineOption restoreDefault("restore-default-terminal","Restore the previous terminal preference"); parser.addOption(restoreDefault);
    parser.addOption(QCommandLineOption({"e","execute"},"Execute program with all remaining arguments (place last)","program"));
    QCommandLineOption adminCommand("admin-command","Base64 UTF-8 command to run in an elevated session","base64"); parser.addOption(adminCommand);
    QCommandLineOption adminEngine("admin-engine","Native elevated engine: powershell or cmd","engine"); parser.addOption(adminEngine);
    auto applicationArguments=app.arguments();
    const auto command=takeCommandArguments(applicationArguments);
    const bool executionRequested=applicationArguments.size()!=app.arguments().size();
    parser.process(applicationArguments);
    if (parser.isSet(setDefault)||parser.isSet(defaultStatus)||parser.isSet(restoreDefault))
        return defaultTerminalCommand({parser.isSet(restoreDefault)?"restore":parser.isSet(defaultStatus)?"status":"set"},app.applicationFilePath());
    if (parser.isSet(resetTextSize)) {
        Store store; auto appearance=store.appearance();
        appearance.font.setPointSizeF(Appearance().font.pointSizeF()); store.saveAppearance(appearance);
    }
    if (executionRequested && command.isEmpty()) { std::cerr << "-e requires a program and optional arguments.\n"; return 2; }
    if (executionRequested && (parser.isSet(adminCommand) || parser.isSet(adminEngine) || parser.isSet(profile))) { std::cerr << "-e cannot be combined with --profile, --admin-command or --admin-engine.\n"; return 2; }
    if ((parser.isSet(adminCommand) || parser.isSet(adminEngine)) && !isAdministrator()) { std::cerr << "An admin command requires a Windows-elevated process. Use admin or /admin.\n"; return 4; }
    std::optional<Profile> initialProfile;
    if (executionRequested) initialProfile=Profile{QFileInfo(command.first()).fileName(),{command.first(),command.mid(1),parser.isSet(directory)?parser.value(directory):QDir::currentPath()}};
    if (parser.isSet(adminEngine) || parser.isSet(adminCommand)) {
        QString error;
        const auto code=QString::fromUtf8(QByteArray::fromBase64(parser.value(adminCommand).toLatin1()));
        initialProfile=nativeAdminProfile(parser.isSet(adminEngine)?parser.value(adminEngine):"powershell",code,parser.isSet(directory)?parser.value(directory):defaultWorkingDirectory(),error);
        if (!error.isEmpty()) { std::cerr << error.toStdString() << '\n'; return 4; }
    }
    if (!initialProfile && !executionRequested && parser.isSet(directory)) {
        QStringList args;
        auto cwd=parser.value(directory); if (cwd.isEmpty()) cwd=defaultWorkingDirectory();
        initialProfile=Profile{isAdministrator()?"Administrator":"Terlox shell",{QCoreApplication::applicationDirPath()+
#ifdef Q_OS_WIN
            "/terloxsh.exe"
#else
            "/terloxsh"
#endif
            ,args,cwd}};
    }
    Window window(parser.value(profile),nullptr,initialProfile?&*initialProfile:nullptr);
    if (parser.isSet(diagnostics)) {
        auto* view=window.findChild<TerminalView*>();
        if (!view) return 2;
        QObject::connect(view->quickWindow(),&QQuickWindow::afterRendering,&app,[&] {
            const auto report=window.informationText().toUtf8();
            if (parser.isSet(diagnosticsOutput)) {
                QSaveFile file(parser.value(diagnosticsOutput));
                if (!file.open(QIODevice::WriteOnly) || file.write(report)!=report.size() || !file.commit()) { app.exit(3); return; }
            } else std::cout << report.constData() << std::flush;
            app.exit(0);
        },Qt::ConnectionType(Qt::QueuedConnection|Qt::SingleShotConnection));
        QTimer::singleShot(10000,&app,[&] { app.exit(2); });
    }
    window.show();
    if (parser.isSet("update-result")) {
        QFile report(parser.value("update-result"));
        if (report.open(QIODevice::ReadOnly) && report.size()<8192) {
            const auto result=QJsonDocument::fromJson(report.readAll()).object();
            if (result.value("success").isBool() && !result.value("success").toBool()) {
                const auto message=result.value("message").toString()+"\n\nReport: "+report.fileName();
                QTimer::singleShot(0,&window,[&window,message] { QMessageBox::warning(&window,"Terlox update",message); });
            }
        }
    }
    return app.exec();
}
}
