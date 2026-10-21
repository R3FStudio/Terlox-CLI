#include "AdminProfile.h"
#include "platform/AdminLaunch.h"
namespace terlox {
Profile nativeAdminProfile(const QString& engine,const QString& command,const QString& directory,QString& error) {
#ifdef Q_OS_WIN
    const auto name=QString("Administrator");
#else
    const auto name=QString("Root session (sudo)");
#endif
    return {name,nativeAdminLaunch(engine,command,directory,error)};
}
}
