#include "wizard/SetupWizard.h"
#include "branding/Brand.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <iostream>
int main(int argc,char** argv) {
    QApplication app(argc,argv); app.setApplicationName("Terlox Setup"); app.setOrganizationName("Terlox"); app.setWindowIcon(terlox::brandIcon());
    QCommandLineParser parser; parser.setApplicationDescription("Install an extracted Terlox release for the current account"); parser.addHelpOption();
    parser.addOption({"package","Extracted Terlox release directory","directory"});
    parser.addOption({"destination","Initial installation folder","directory"});
    parser.addOption({"check-package","Validate the package and destination, without installing"}); parser.process(app);
    auto source=parser.value("package");
    if (source.isEmpty()) {
        QDir candidate(QCoreApplication::applicationDirPath());
        for (int i=0;i<5;++i) { if (QFileInfo::exists(candidate.filePath(".terlox-package.json"))) break; if (!candidate.cdUp()) break; }
        source=candidate.absolutePath();
    }
    const auto destination=parser.isSet("destination")?parser.value("destination"):terlox::setup::defaultDestination();
    if (parser.isSet("check-package")) {
        terlox::setup::InstallPlan plan; QString error;
        if (!terlox::setup::buildInstallPlan({source,destination,false,false,false},plan,error)) { std::cerr << error.toStdString() << '\n'; return 2; }
        std::cout << "Terlox " << plan.version.toStdString() << ": " << plan.files.size() << " files ready for installation.\n"; return 0;
    }
    terlox::setup::SetupWizard wizard(source); wizard.setField("destination",destination); wizard.show(); return app.exec();
}
