#include "Window.h"
#include "AnimatedMenu.h"
#include "ZoomControls.h"
#include "information/Shortcuts.h"
#include "platform/Elevation.h"
#include <QStandardPaths>
#include <QShortcut>
#include <QMenu>
#include <QWindow>
#include <QWidgetAction>
namespace terlox {
void Window::zoomText(int steps) {
    textZoom_=std::clamp(appearance_.font.pointSizeF()+textZoom_+steps,6.,40.)-appearance_.font.pointSizeF();
    applyAppearance(false);
}
void Window::installShortcuts() {
    for (const auto& binding:shortcuts()) {
        auto* shortcut=new QShortcut(QKeySequence(binding.keys),this);
        connect(shortcut,&QShortcut::activated,this,[this,action=binding.action] {
            switch (action) {
            case Action::NewSession: newDefaultSession(); break;
            case Action::CloseSession: closeTab(tabs_->currentIndex()); break;
            case Action::NextSession: if(tabs_->count()) tabs_->setCurrentIndex((tabs_->currentIndex()+1)%tabs_->count()); break;
            case Action::PreviousSession: if(tabs_->count()) tabs_->setCurrentIndex((tabs_->currentIndex()+tabs_->count()-1)%tabs_->count()); break;
            case Action::Copy: if(current()) current()->copy(); break;
            case Action::Paste: if(current()) current()->paste(); break;
            case Action::LargerFont: zoomText(1); break;
            case Action::SmallerFont: zoomText(-1); break;
            case Action::Maximize: (isMaximized() || isFullScreen())?showNormal():showMaximized(); break;
            case Action::Minimize: showMinimized(); break;
            case Action::Menu: showMenu(mapToGlobal(rect().center())); break;
            case Action::Information: showInformation(); break;
            }
        });
    }
}
void Window::showMenu(const QPoint& pos) {
    AnimatedMenu menu(this); menu.setObjectName("terminalContextMenu"); menu.addAction("Copy\tCtrl+C", this, [this] { if (current()) current()->copy(); });
    menu.addAction("Paste\tCtrl+V", this, [this] { if (current()) current()->paste(); });
    menu.addSeparator();
    auto* zoom=new ZoomControls(&menu);
    auto* zoomAction=new QWidgetAction(&menu); zoomAction->setDefaultWidget(zoom); menu.addAction(zoomAction);
    zoom->setZoom(displayAppearance().font.pointSizeF(),appearance_.font.pointSizeF());
    connect(zoom,&ZoomControls::stepRequested,this,&Window::zoomText);
    connect(zoom,&ZoomControls::resetRequested,this,[this] { textZoom_=0; applyAppearance(false); });
    connect(this,&Window::textZoomChanged,zoom,&ZoomControls::setZoom);
    menu.addSeparator();
    auto* shells = new AnimatedMenu("New session",&menu); menu.addMenu(shells);
    for (const auto& profile : profiles_) shells->addAction(profile.name, this, [this, profile] { newSession(profile); });
#ifdef Q_OS_WIN
    if (isAdministrator()) {
        auto* active=menu.addAction("Administrator mode (active)"); active->setEnabled(false); active->setObjectName("administratorModeActive");
    } else {
        auto* admin=new AnimatedMenu("Run as administrator",&menu); admin->setObjectName("administratorSessionMenu");
        menu.addMenu(admin);
        auto* powershell=admin->addAction("PowerShell",this,[this] { openAdminSession("powershell"); }); powershell->setObjectName("administratorPowerShell");
        auto* cmd=admin->addAction("Command Prompt",this,[this] { openAdminSession("cmd"); }); cmd->setObjectName("administratorCommandPrompt");
    }
#else
#ifdef Q_OS_MACOS
    const auto adminLabel=QString("Open administrator session (sudo)");
#else
    const auto adminLabel=QString("Open root session (sudo)");
#endif
    auto* admin=menu.addAction(adminLabel,this,[this] { openAdminSession(); }); admin->setObjectName("administratorSession");
    admin->setEnabled(!QStandardPaths::findExecutable("sudo").isEmpty());
    admin->setToolTip("Authenticate with sudo in a new native shell session");
#endif
    menu.addSeparator();
    menu.addAction("Close session\tCtrl+Shift+W", this, [this] { closeTab(tabs_->currentIndex()); });
    menu.addAction("Quit", this, &QWidget::close); menu.exec(pos); if (current()) current()->setFocus();
}
}
