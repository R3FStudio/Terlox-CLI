#pragma once
#include "platform/LaunchOptions.h"
#include <QList>
namespace terlox {
struct Profile { QString name; LaunchOptions launch; };
QString defaultWorkingDirectory();
QList<Profile> discoverProfiles(const QString& applicationDirectory);
}
