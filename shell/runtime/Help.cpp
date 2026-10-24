#include "Help.h"
namespace terlox::shell {
QString shellHelp() {
    return "Terlox shell\n"
           "Usage: terloxsh [-c command | script.tlx | --command-and-stay command]\n"
           "       terloxsh --help | --version\n\n"
           "cd [path]              Change folder (cd ~ goes home)\n"
           "pwd                    Print current folder\n"
           "echo [text]            Print text\n"
           "clear                  Clear the visible screen\n"
           "exit [status]          Leave the shell\n"
           "export/set NAME=value  Set environment variables\n"
           "unset NAME             Remove an environment variable\n"
           "env                    List environment variables\n"
           "history                List interactive command history\n"
           "help                   Show this command list\n"
           "terlox-default [set|status|restore]  OS default terminal preference\n"
           "terlox-update [check]  Check releases; confirm to install and restart\n"
           "admin --help           Explicit administrator-session options\n"
#ifdef Q_OS_WIN
           "sudo command           Request Windows UAC for a command\n"
#else
           "sudo command           Use your installed sudo and its password prompt\n"
#endif
           "/info-terlox           Terminal information\n\n"
           "Quotes: 'literal', \"expanded $NAME\". Last status: $?\n"
           "Pipelines: a | b. Files: <, >, >>, 2>. Statements: ;\n"
           "Use the installed native shell for its full scripting language.\n"
           "Inside Terlox: /terlox help lists window settings, including border color.\n";
}
}
