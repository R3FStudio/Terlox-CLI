#pragma once
#include <QStringList>
namespace terlox::shell {
struct ShellState { int status = 0; bool exiting = false; };
bool isBuiltin(const QString& name);
int builtin(const QStringList& args, ShellState& state);
}
