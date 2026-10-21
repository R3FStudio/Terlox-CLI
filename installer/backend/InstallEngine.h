#pragma once
#include "installer/model/InstallPlan.h"
#include <functional>
namespace terlox::setup {
struct InstallResult { bool success=false; QString message; QStringList notes; };
using Progress=std::function<void(int,const QString&)>;
InstallResult installPackage(const InstallPlan& plan,const Progress& progress={});
}
