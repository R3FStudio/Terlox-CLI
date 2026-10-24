#pragma once
#include <QStringList>
#include <iosfwd>
namespace terlox {
bool confirmUpdate(std::istream& input,std::ostream& output);
int updateCommand(const QStringList& arguments);
}
