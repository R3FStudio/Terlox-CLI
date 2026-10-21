#include "LaunchArguments.h"
namespace terlox {
QStringList takeCommandArguments(QStringList& arguments) {
    for (int i=1;i<arguments.size();++i) {
        const auto& arg=arguments[i];
        if (arg=="-e" || arg=="--execute" || arg=="--") {
            const auto command=arguments.mid(i+1);
            arguments=arguments.mid(0,i); return command;
        }
        if (arg=="--directory" || arg=="--working-directory" || arg=="--profile" || arg=="--admin-engine" || arg=="--admin-command" || arg=="--diagnostics-output") ++i;
    }
    return {};
}
}
