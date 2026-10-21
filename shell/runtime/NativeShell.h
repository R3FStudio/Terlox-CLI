#pragma once
#include <QStringList>
#include <QProcess>
#include <optional>
#include <functional>
#include "platform/Elevation.h"
namespace terlox::shell {
using ElevationLauncher=std::function<bool(const ElevationRequest&,QString&)>;
std::optional<int> nativeShellLine(const QString& line,const ElevationLauncher& launcher={});
void configureNativeProgram(QProcess& process, const QString& executable, const QStringList& arguments);
}
