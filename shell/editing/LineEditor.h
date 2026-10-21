#pragma once
#include "History.h"
#include <optional>
namespace terlox::shell { std::optional<QString> readLine(History& history); bool isInteractive(); }
