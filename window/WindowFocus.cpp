#include "Window.h"
#include "platform/Backdrop.h"
#include "platform/WindowActivation.h"
#include <QApplication>
#include <QEvent>
#include <QTimer>

namespace terlox {
QColor Window::frameColor() const {
    if (!isActiveWindow()) return QColor("#3b3b40");
    return current()?current()->effectiveAppearance().border:appearance_.border;
}

void Window::focusTerminal() {
    // Activation can arrive after showEvent and the native render surface.
    // Repair input focus only inside our active window, never over a popup,
    // dialog or another application the user has switched to.
    if (!isVisible() || isMinimized() || !isActiveWindow()
        || QApplication::activePopupWidget() || QApplication::activeModalWidget()) return;
    if (auto* view=current()) view->setFocus(Qt::ActiveWindowFocusReason);
}

void Window::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type()!=QEvent::ActivationChange) return;
    applyWindowBorder(winId(),frameColor()); update();
    if (!isActiveWindow()) return;
    focusTerminal();
    // Qt may finish restoring its previous focus widget after activation.
    QTimer::singleShot(0,this,&Window::focusTerminal);
}

void Window::showEvent(QShowEvent*) {
    applySessionSurface();
    if (initialActivationRequested_) return;
    initialActivationRequested_=true;
    QTimer::singleShot(0,this,[this] {
        if (!isVisible() || isMinimized()) return;
        bringWindowToFront(windowHandle());
        focusTerminal();
    });
}
}
