#include "Command.h"
#include "Client.h"
#include "Service.h"
#include "information/Version.h"
#include <iostream>

namespace terlox {
bool confirmUpdate(std::istream& input,std::ostream& output) {
    output << "Update now? Terlox will close its sessions, install the update, and restart. [yes/no]: " << std::flush;
    std::string answer;
    return std::getline(input,answer) && QString::fromStdString(answer).trimmed().compare("yes",Qt::CaseInsensitive)==0;
}
int updateCommand(const QStringList& arguments) {
    if (arguments==QStringList{"--help"} || arguments==QStringList{"help"}) {
        std::cout << "terlox-update [check]\nterloxctl update [check]\nCheck stable releases; type yes when prompted to install and restart.\nThe check argument only checks and never installs.\n"; return 0;
    }
    if (!arguments.isEmpty() && arguments!=QStringList{"check"}) { std::cerr << "Usage: terlox-update [check] or terloxctl update [check]\n"; return 2; }
    std::cout << "Checking R3FStudio/Terlox-CLI for updates...\n" << std::flush;
    const auto result=updates::checkLatest();
    if (result.state!=updates::CheckState::Release) {
        std::cout << result.message.toUtf8().constData() << "\nhttps://github.com/R3FStudio/Terlox-CLI/releases\n";
        return result.state==updates::CheckState::Error?1:0;
    }
    const auto current=QVersionNumber::fromString(TERLOX_VERSION);
    const auto comparison=QVersionNumber::compare(result.release.version,current);
    std::cout << "Installed: " << TERLOX_VERSION << "\nLatest stable: " << result.release.tag.toUtf8().constData() << '\n';
    std::cout << (comparison>0?"An update is available.\n":comparison==0?"Terlox is up to date.\n":"This build is newer than the latest published stable release.\n");
    std::cout << result.release.page.toString().toUtf8().constData() << '\n';
    if (comparison>0) {
        if (result.release.downloads.isEmpty()) std::cout << "No installer matches this platform yet. Check the release page.\n";
        for (const auto& url:result.release.downloads) std::cout << url.toString().toUtf8().constData() << '\n';
        if (arguments==QStringList{"check"} || result.release.downloads.isEmpty()) return 0;
        if (!confirmUpdate(std::cin,std::cout)) {
            std::cout << "Update cancelled.\n"; return 0;
        }
        QString error;
        if (!updates::requestInstallation(error)) { std::cerr << error.toUtf8().constData() << '\n'; return 1; }
    }
    return 0;
}
}
