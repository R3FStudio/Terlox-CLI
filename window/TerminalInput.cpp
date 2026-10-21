#include "TerminalView.h"
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QInputMethodEvent>
#include <QWindow>
#include <QApplication>
namespace terlox {
void TerminalView::keyPressEvent(QKeyEvent* e) {
    if (e->key()==Qt::Key_Control || e->key()==Qt::Key_Shift || e->key()==Qt::Key_Alt || e->key()==Qt::Key_Meta || e->key()==Qt::Key_AltGr) { e->accept(); return; }
    if (e->modifiers()==Qt::ShiftModifier && (e->key()==Qt::Key_Plus || e->key()==Qt::Key_Minus || e->key()==Qt::Key_Underscore)) {
        emit fontZoomRequested(e->key()==Qt::Key_Plus?1:-1); e->accept(); return;
    }
    if (e->modifiers()==(Qt::ControlModifier|Qt::AltModifier) && (e->key()==Qt::Key_A || e->key()==Qt::Key_C || e->key()==Qt::Key_V)) {
        hasSelection_=false; resetScrollback(); typedCommand_.clear(); commandTracking_=e->key()==Qt::Key_C;
        QKeyEvent native(QEvent::KeyPress,e->key(),Qt::ControlModifier); screen_->key(&native); e->accept(); return;
    }
    if (e->modifiers()==Qt::ControlModifier) {
        if (e->key()==Qt::Key_A) { selectAll(); e->accept(); return; }
        if (e->key()==Qt::Key_C && hasSelection_) { copy(); e->accept(); return; }
        if (e->key()==Qt::Key_V) { paste(); e->accept(); return; }
    }
    if (aboutKey(e)) { e->accept(); return; }
    if (hasSelection_ && (e->key()==Qt::Key_Backspace || e->key()==Qt::Key_Delete) && e->modifiers()==Qt::NoModifier) {
        deleteSelectedInput(); e->accept(); return;
    }
    const auto command=typedCommand_.trimmed();
    if (!screen_->alternate() && (e->key()==Qt::Key_Return || e->key()==Qt::Key_Enter) && commandTracking_
        && (command=="/terlox" || command.startsWith("/terlox ") || command=="/admin" || command.startsWith("/admin "))) {
        typedCommand_.clear(); commandTracking_=true;
        QKeyEvent cancel(QEvent::KeyPress,Qt::Key_C,Qt::ControlModifier); screen_->key(&cancel);
        emit applicationCommandRequested(command); e->accept(); return;
    }
    if (!screen_->alternate() && (e->key()==Qt::Key_Return || e->key()==Qt::Key_Enter) && commandTracking_ && (typedCommand_.trimmed()=="/info-terlox" || typedCommand_.simplified().compare("About Terlox",Qt::CaseInsensitive)==0)) {
        typedCommand_.clear(); commandTracking_=true;
        QKeyEvent cancel(QEvent::KeyPress,Qt::Key_C,Qt::ControlModifier); screen_->key(&cancel);
        emit informationRequested(); return;
    }
    if ((e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) == (Qt::ControlModifier | Qt::ShiftModifier)) {
        if (e->key() == Qt::Key_C) { copy(); return; } if (e->key() == Qt::Key_V) { paste(); return; }
    }
    if (e->modifiers() == Qt::ShiftModifier && (e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown)) {
        scrollTo(scrollTarget_+(e->key()==Qt::Key_PageUp?1:-1)*screen_->rows()); return;
    }
    if (e->key()==Qt::Key_Return || e->key()==Qt::Key_Enter || (e->modifiers()==Qt::ControlModifier && e->key()==Qt::Key_C)) { typedCommand_.clear(); commandTracking_=true; }
    else if (e->key()==Qt::Key_Backspace && commandTracking_) { if (!typedCommand_.isEmpty()) typedCommand_.chop(1); }
    else if (e->key()==Qt::Key_Escape || e->key()==Qt::Key_Up || e->key()==Qt::Key_Down || e->key()==Qt::Key_Left || e->key()==Qt::Key_Right || e->key()==Qt::Key_Home || e->key()==Qt::Key_End || e->key()==Qt::Key_Delete || e->modifiers().testFlag(Qt::ControlModifier)) commandTracking_=false;
    else if (commandTracking_) typedCommand_+=e->text();
    hasSelection_ = false; resetScrollback(); cursorOn_ = true; blink_.start(); screen_->key(e); refreshScene();
}
void TerminalView::mousePressEvent(QMouseEvent* e) {
    dragging_=false;
    if (e->button()==Qt::LeftButton || e->button()==Qt::RightButton) setFocus(Qt::MouseFocusReason);
    if (screen_->mouseReporting() && !e->modifiers().testFlag(Qt::ShiftModifier)) {
        const int button=e->button()==Qt::LeftButton?1:e->button()==Qt::MiddleButton?2:e->button()==Qt::RightButton?3:0;
        if (button) screen_->mouse(cellAt(e->position()),button,true,e->modifiers()); e->accept(); return;
    }
    if (e->button() != Qt::LeftButton) return;
    anchor_ = extent_ = cellAt(e->position()); dragging_ = true; hasSelection_ = false; refreshScene();
    e->accept();
}
void TerminalView::mouseDoubleClickEvent(QMouseEvent* e) { mousePressEvent(e); }
void TerminalView::mouseMoveEvent(QMouseEvent* e) {
    if (screen_->mouseReporting() && !e->modifiers().testFlag(Qt::ShiftModifier)) { screen_->mouseMove(cellAt(e->position()),e->modifiers()); setCursor(Qt::IBeamCursor); e->accept(); return; }
    if (dragging_ && e->buttons().testFlag(Qt::LeftButton)) { extent_ = cellAt(e->position()); hasSelection_ = extent_ != anchor_; refreshScene(); }
    setCursor(Qt::IBeamCursor);
}
void TerminalView::mouseReleaseEvent(QMouseEvent* e) {
    if (screen_->mouseReporting() && !e->modifiers().testFlag(Qt::ShiftModifier)) { const int button=e->button()==Qt::LeftButton?1:e->button()==Qt::MiddleButton?2:3; screen_->mouse(cellAt(e->position()),button,false,e->modifiers()); } dragging_ = false; setCursor(Qt::IBeamCursor); }
void TerminalView::wheelEvent(QWheelEvent* e) {
    if (e->modifiers().testFlag(Qt::ControlModifier)) {
        zoomWheelDelta_+=e->angleDelta().y();
        if (e->angleDelta().isNull() && e->pixelDelta().y()) zoomWheelDelta_+=e->pixelDelta().y()*4;
        const int steps=zoomWheelDelta_/120; zoomWheelDelta_%=120;
        if (steps) emit fontZoomRequested(steps);
        e->accept(); return;
    }
    zoomWheelDelta_=0;
    if (aboutScreen_) { aboutScroll_=std::max(0,aboutScroll_-e->angleDelta().y()/40); renderAbout(); e->accept(); return; }
    if (screen_->mouseReporting() && !e->modifiers().testFlag(Qt::ShiftModifier)) { const int button=e->angleDelta().y()>0?4:5; screen_->mouse(cellAt(e->position()),button,true,e->modifiers()); screen_->mouse(cellAt(e->position()),button,false,e->modifiers()); e->accept(); return; }
    if (screen_->alternate()) { for (int i = 0; i < 3; ++i) { QKeyEvent key(QEvent::KeyPress, e->angleDelta().y() > 0 ? Qt::Key_Up : Qt::Key_Down, Qt::NoModifier); screen_->key(&key); } return; }
    const qreal lines=e->pixelDelta().y()?e->pixelDelta().y()/cellHeight_:e->angleDelta().y()/40.;
    scrollTo(scrollTarget_+lines); e->accept();
}
void TerminalView::contextMenuEvent(QContextMenuEvent* e) { if (screen_->mouseReporting() && !e->modifiers().testFlag(Qt::ShiftModifier)) { e->accept(); return; } dragging_=false; setFocus(Qt::MouseFocusReason); emit menuRequested(e->globalPos()); e->accept(); }
void TerminalView::inputMethodEvent(QInputMethodEvent* e) { if (!e->commitString().isEmpty()) { if (commandTracking_) typedCommand_+=e->commitString(); QKeyEvent key(QEvent::KeyPress, 0, Qt::NoModifier, e->commitString()); screen_->key(&key); } e->accept(); }
QVariant TerminalView::inputMethodQuery(Qt::InputMethodQuery q) const { if (q == Qt::ImCursorRectangle) { auto c = screen_->cursor(); return QRectF(c.col*cellWidth_, c.row*cellHeight_, cellWidth_, cellHeight_); } return QWidget::inputMethodQuery(q); }
}
