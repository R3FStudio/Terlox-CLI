#include "installer/backend/InstallEngine.h"
#include "installer/uninstall/UninstallPlan.h"
#include "installer/windows/InstalledApplication.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QSaveFile>
#include <iostream>
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv); app.setApplicationName("Terlox maintenance"); app.setOrganizationName("Terlox");
    QCommandLineParser parser; parser.addHelpOption();
    parser.addOption({"uninstall","Remove the installation inventory and its Windows registration"});
    parser.addOption({"install","Install a deployed package without desktop shortcuts"});
    parser.addOption({"repair-uninstaller","Add uninstall support to an existing installation"});
    parser.addOption({"directory","Installation folder","path"});
    parser.addOption({"package","Deployed package for installation","path"});
    parser.addOption({"report","Write a UTF-8 result report","path"}); parser.process(app);
    QString error; QStringList notes; bool success=false;
    const int actions=int(parser.isSet("install"))+int(parser.isSet("uninstall"))+int(parser.isSet("repair-uninstaller"));
    if (!parser.isSet("directory") || actions!=1) error="Specify --directory and exactly one maintenance action.";
    else if (parser.isSet("repair-uninstaller")) success=terlox::setup::repairUninstaller(parser.value("package"),parser.value("directory"),error);
    else if (parser.isSet("install")) {
        terlox::setup::InstallPlan plan;
        if (terlox::setup::buildInstallPlan({parser.value("package"),parser.value("directory"),false,false,false,true},plan,error)) {
            const auto result=terlox::setup::installPackage(plan); success=result.success; error=result.message; notes=result.notes;
        }
    } else success=terlox::setup::uninstallPackage(parser.value("directory"),error,notes);
    const auto report=(success?QString("Terlox maintenance completed successfully."):error)+'\n'+notes.join('\n');
    (success?std::cout:std::cerr) << report.toUtf8().constData() << '\n';
    if (parser.isSet("report")) { QSaveFile file(parser.value("report")); const auto bytes=report.toUtf8(); if (file.open(QIODevice::WriteOnly)) { file.write(bytes); file.commit(); } }
    return success?0:2;
}
