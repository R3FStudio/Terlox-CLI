#pragma once
#include <QStringList>
#include <vector>
namespace terlox::shell {
struct Command {
    QStringList arguments;
    QString input, output, error;
    bool append = false;
};
using Pipeline = std::vector<Command>;
using Program = std::vector<Pipeline>;
struct ParseResult { Program program; QString error; };
}
