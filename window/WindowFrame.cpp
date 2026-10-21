#include "Window.h"
#include <QApplication>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QWindow>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace terlox {
#ifdef Q_OS_WIN
bool Window::nativeEvent(const QByteArray& type,void* message,qintptr* result) {
    const auto* event=static_cast<MSG*>(message);
    // Windows paints its classic minimized caption even for a layered popup.
    // All of Terlox's frame is client-rendered, so that GDI painting must not
    // touch the alpha surface during minimize/restore or activation changes.
    if (event->message==WM_NCPAINT || event->message==WM_NCACTIVATE) {
        *result=event->message==WM_NCACTIVATE?TRUE:0; return true;
    }
    return QWidget::nativeEvent(type,message,result);
}
#endif
Qt::Edges Window::edges(const QPointF& p) const {
    Qt::Edges e; if (isMaximized() || isFullScreen()) return e;
    if (p.x()<8) e|=Qt::LeftEdge; if (p.x()>width()-8) e|=Qt::RightEdge;
    if (p.y()<8) e|=Qt::TopEdge; if (p.y()>height()-8) e|=Qt::BottomEdge; return e;
}
bool Window::dragArea(const QPointF& p) const { return rect().contains(p.toPoint()) && p.y()<TopInset && !edges(p); }
void Window::mousePressEvent(QMouseEvent* e) {
    if (e->button()!=Qt::LeftButton) return;
    const auto edge=edges(e->position());
    if (edge) { windowHandle()->startSystemResize(edge); e->accept(); return; }
    if (dragArea(e->position())) { windowHandle()->startSystemMove(); e->accept(); }
}
void Window::mouseMoveEvent(QMouseEvent* e) {
    const auto edge=edges(e->position());
    if ((edge&(Qt::LeftEdge|Qt::RightEdge)) && (edge&(Qt::TopEdge|Qt::BottomEdge)))
        setCursor(((edge&Qt::LeftEdge) && (edge&Qt::TopEdge)) || ((edge&Qt::RightEdge) && (edge&Qt::BottomEdge))?Qt::SizeFDiagCursor:Qt::SizeBDiagCursor);
    else if (edge&(Qt::LeftEdge|Qt::RightEdge)) setCursor(Qt::SizeHorCursor);
    else if (edge&(Qt::TopEdge|Qt::BottomEdge)) setCursor(Qt::SizeVerCursor);
    else unsetCursor();
}
void Window::mouseReleaseEvent(QMouseEvent* e) { e->accept(); }
void Window::mouseDoubleClickEvent(QMouseEvent* e) {
    e->accept();
}
void Window::contextMenuEvent(QContextMenuEvent* e) { showMenu(e->globalPos()); e->accept(); }
}
