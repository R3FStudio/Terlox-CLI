#pragma once
#include <QStringList>
namespace terlox {
// Everything after the execution delimiter is an argv vector, never shell text.
QStringList takeCommandArguments(QStringList& applicationArguments);
}
