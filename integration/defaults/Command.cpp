#include "Command.h"
#include "DefaultTerminal.h"
#include <iostream>
namespace terlox {
int defaultTerminalCommand(const QStringList& arguments,const QString& executable) {
    const auto action=arguments.value(0,"set");
    if (arguments.size()>1 || !QStringList{"set","status","restore"}.contains(action)) {
        std::cerr << "Usage: terloxctl default-terminal [set|status|restore]\n"; return 2;
    }
    const auto result=action=="status"?defaultTerminalStatus():action=="restore"?restoreDefaultTerminal():selectDefaultTerminal(executable);
    (result.success?std::cout:std::cerr) << result.message.toUtf8().constData() << '\n';
    return result.success?0:1;
}
}
