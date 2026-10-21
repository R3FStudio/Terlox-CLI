#pragma once
#include <QProcessEnvironment>
namespace terlox {
QProcessEnvironment terminalEnvironment();
QProcessEnvironment interactiveTerminalEnvironment();
void importTerminalEnvironment();
}
