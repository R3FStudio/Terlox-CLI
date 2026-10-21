#include "Window.h"
#include "platform/Elevation.h"
#include "settings/AdminProfile.h"
#include <QCoreApplication>
namespace terlox {
void Window::openAdminSession(const QString& engine,const QString& command) {
    auto* view=current(); if (!view) return;
    auto directory=view->property("profileDirectory").toString();
    if (directory.isEmpty()) directory=defaultWorkingDirectory();
    QString error;
#ifdef Q_OS_WIN
    const bool opened=elevateTerminal(elevatedTerminalRequest(QCoreApplication::applicationDirPath(),command,directory,engine),error,winId());
#else
    const auto profile=nativeAdminProfile({},command,directory,error);
    const bool opened=error.isEmpty(); if (opened) newSession(profile);
#endif
    if (!opened) { view->screen()->feed(("\r\n"+error+"\r\n").toUtf8()); view->setFocus(); }
}
}
