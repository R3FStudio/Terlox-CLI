#include "Command.h"
#include "Client.h"
#include "information/Version.h"
#include <iostream>

namespace terlox {
int updateCommand(const QStringList& arguments) {
    if (arguments==QStringList{"--help"} || arguments==QStringList{"help"}) {
        std::cout << "terlox-update [check]\nterloxctl update [check]\nCheck the latest public stable Terlox release. No files are installed.\n"; return 0;
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
        std::cout << "Download the appropriate package, close Terlox, and run the installer or replace your AppImage.\n";
    }
    return 0;
}
}
