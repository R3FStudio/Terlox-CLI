#pragma once
#include "installer/model/InstallPlan.h"
namespace terlox::setup {
bool registerDesktop(const InstallPlan& plan,QString& error,QStringList& notes);
}
