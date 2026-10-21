#include "Backdrop.h"
#include "window/Window.h"
#include "branding/Brand.h"
#include "platform/Environment.h"
#include "renderer/GraphicsBackend.h"
#include "tests/CompositorCapture.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QScreen>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QTest>
#include <memory>
int main(int argc,char** argv) {
    terlox::configureGraphics(); terlox::importTerminalEnvironment();
    QApplication app(argc,argv); app.setApplicationName("Terlox"); app.setOrganizationName("Terlox"); app.setWindowIcon(terlox::brandIcon());
    QCommandLineParser parser; parser.addHelpOption();
    parser.addOption({"output","Composited PNG to save","file"}); parser.addOption({"python","Python executable","file"});
    parser.addOption({"project","Project folder containing examples/showcase.py","folder"}); parser.process(app);
    if (parser.value("output").isEmpty() || parser.value("python").isEmpty() || parser.value("project").isEmpty()) return 2;
    PresentationBackdrop backdrop;
    const auto available=app.primaryScreen()->availableGeometry();
    const int width=std::min({1440,available.width()-80,int((available.height()-80)*1.6)}),height=int(width/1.6);
    backdrop.setGeometry(available.center().x()-width/2,available.center().y()-height/2,width,height); backdrop.show();
    terlox::Profile profile{"Terlox showcase",{parser.value("python"),{QDir(parser.value("project")).filePath("examples/showcase.py")},parser.value("project")}};
    terlox::Window terminal({},nullptr,&profile);
    terminal.setGeometry(backdrop.x()+int(width*.1),backdrop.y()+int(height*.15),int(width*.8),int(height*.7)); terminal.show(); terminal.raise(); terminal.activateWindow();
    auto* view=terminal.findChild<terlox::TerminalView*>();
    auto* poll=new QTimer(&app); poll->setInterval(100);
    QObject::connect(poll,&QTimer::timeout,&app,[&] {
        QString text; for (int row=0;row<view->screen()->rows();++row) text+=view->screen()->text(row,0,view->screen()->columns()-1);
        if (!text.contains("Press Enter to return")) return;
        poll->stop();
        QTimer::singleShot(900,&app,[&] {
            // Capture the real Windows desktop pixels covering both owned
            // windows. No QWidget grab, fake alpha layer, or post-processing.
            const auto image=terlox::testing::captureDesktopWindow(&backdrop);
            QDir().mkpath(QFileInfo(parser.value("output")).absolutePath());
            app.exit(!image.isNull() && image.save(parser.value("output"))?0:3);
        });
    }); poll->start(); QTimer::singleShot(15000,&app,[&] { app.exit(4); }); return app.exec();
}
