#include "integration/defaults/Command.h"
#include "updates/Command.h"
#include <QCoreApplication>
#include <QDir>
#include <iostream>
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv); app.setApplicationName("Terlox"); app.setOrganizationName("Terlox");
    const auto args=app.arguments().mid(1);
    if (args.value(0)=="update") return terlox::updateCommand(args.mid(1));
    if (args.value(0)!="default-terminal") { std::cerr << "Usage: terloxctl default-terminal [set|status|restore]\n       terloxctl update [check]\n"; return 2; }
    return terlox::defaultTerminalCommand(args.mid(1),QDir(app.applicationDirPath()).filePath(
#ifdef Q_OS_WIN
        "terlox.exe"
#else
        "terlox"
#endif
    ));
}
