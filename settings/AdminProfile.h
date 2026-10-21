#pragma once
#include "Profiles.h"
namespace terlox {
Profile nativeAdminProfile(const QString& engine,const QString& command,const QString& directory,QString& error);
}
