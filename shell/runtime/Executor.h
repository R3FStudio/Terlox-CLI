#pragma once
#include "Builtins.h"
#include "shell/parsing/Command.h"
namespace terlox::shell {
int execute(const Pipeline& pipeline, ShellState& state);
int run(const QString& line, ShellState& state);
}
