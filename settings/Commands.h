#pragma once
#include "Appearance.h"
#include <QStringList>
namespace terlox {
struct AppearanceResult { bool changed=false; QString message; };
AppearanceResult appearanceCommand(const QStringList& arguments, Appearance& appearance);
QString customizationHelp();
}
