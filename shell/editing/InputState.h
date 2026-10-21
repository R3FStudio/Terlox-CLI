#pragma once
#include <QString>
namespace terlox::shell {
void publishInputState(const QString& line,qsizetype cursor);
void finishInputState();
}
