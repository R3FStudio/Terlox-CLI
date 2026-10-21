#pragma once
#include <QString>
namespace terlox::shell {
QString expandVariable(const QString& text, qsizetype& position, int lastStatus);
}
