#include "TerminalView.h"
#include <QScrollBar>
#include <QSignalBlocker>
#include <QWheelEvent>
#include <cmath>
namespace terlox {
void TerminalView::initializeScrollBar() {
    scrollBar_=new QScrollBar(Qt::Vertical,this); scrollBar_->setObjectName("terminalScrollBar");
    scrollBar_->setAccessibleName("Terminal history"); scrollBar_->setFocusPolicy(Qt::NoFocus);
    scrollBar_->setCursor(Qt::ArrowCursor); scrollBar_->hide(); scrollBar_->installEventFilter(this);
    scrollBar_->setStyleSheet(
        "QScrollBar:vertical { background:rgba(255,255,255,10); width:8px; border:none; border-radius:4px; margin:0; }"
        "QScrollBar::handle:vertical { background:#85505e; min-height:28px; border-radius:4px; }"
        "QScrollBar::handle:vertical:hover { background:#ba5666; } QScrollBar::handle:vertical:pressed { background:#de8c9e; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; border:none; background:transparent; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background:transparent; }");
    connect(scrollBar_,&QScrollBar::valueChanged,this,[this](int value) { scrollTo(screen_->historySize()-value,false); });
    connect(scrollBar_,&QScrollBar::sliderPressed,this,[this] { setFocus(Qt::MouseFocusReason); });
    scrollAnimation_.setDuration(140); scrollAnimation_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&scrollAnimation_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value) { setScrollPosition(value.toReal()); });
    lastScrollSerial_=screen_->scrollSerial();
}
void TerminalView::setScrollPosition(qreal lines) {
    scrollPosition_=std::clamp(lines,0.,double(screen_->historySize()));
    offset_=int(std::ceil(scrollPosition_)); refreshScene();
}
void TerminalView::scrollTo(qreal lines,bool animated) {
    scrollAnimation_.stop(); scrollTarget_=std::clamp(lines,0.,double(screen_->historySize()));
    if (!animated || qEnvironmentVariableIntValue("TERLOX_REDUCED_MOTION")) { setScrollPosition(scrollTarget_); return; }
    scrollAnimation_.setStartValue(scrollPosition_); scrollAnimation_.setEndValue(scrollTarget_); scrollAnimation_.start();
}
void TerminalView::resetScrollback() {
    scrollAnimation_.stop(); scrollTarget_=scrollPosition_=0; offset_=0;
    lastScrollSerial_=screen_->scrollSerial();
}
void TerminalView::updateScrollBar() {
    const auto delta=screen_->scrollSerial()-lastScrollSerial_; lastScrollSerial_=screen_->scrollSerial();
    if (delta>0) {
        if (hasSelection_) {
            anchor_.ry()-=int(delta); extent_.ry()-=int(delta);
            const int first=-screen_->historySize();
            if (std::max(anchor_.y(),extent_.y())<first) hasSelection_=false;
            else { anchor_.ry()=std::max(first,anchor_.y()); extent_.ry()=std::max(first,extent_.y()); }
        }
        if (scrollPosition_>0) { scrollAnimation_.stop(); scrollPosition_+=delta; scrollTarget_=scrollPosition_; }
    }
    if (screen_->alternate() || aboutScreen_) resetScrollback();
    scrollTarget_=std::clamp(scrollTarget_,0.,double(screen_->historySize()));
    scrollPosition_=std::clamp(scrollPosition_,0.,double(screen_->historySize())); offset_=int(std::ceil(scrollPosition_));
    QSignalBlocker blocker(scrollBar_);
    scrollBar_->setGeometry(width()-11,4,8,std::max(1,height()-8));
    scrollBar_->setRange(0,screen_->historySize()); scrollBar_->setPageStep(screen_->rows());
    scrollBar_->setValue(screen_->historySize()-qRound(scrollPosition_));
    scrollBar_->setVisible(screen_->historySize()>0 && !screen_->alternate() && !aboutScreen_);
    scrollBar_->raise();
}
bool TerminalView::eventFilter(QObject* target,QEvent* event) {
    if (target==scrollBar_ && event->type()==QEvent::Wheel) { wheelEvent(static_cast<QWheelEvent*>(event)); return true; }
    return QQuickWidget::eventFilter(target,event);
}
}
