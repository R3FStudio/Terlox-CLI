#include "AnimatedMenu.h"
#include <QPainter>
#include <QPropertyAnimation>
#include <QShowEvent>
namespace terlox {
AnimatedMenu::AnimatedMenu(QWidget* parent):AnimatedMenu(QString(),parent) {}
AnimatedMenu::AnimatedMenu(const QString& title,QWidget* parent):QMenu(title,parent),animation_(this) {
    setWindowFlags(Qt::Popup|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground); setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    QPalette dark=palette(); dark.setColor(QPalette::Window,QColor("#14151b")); dark.setColor(QPalette::Base,QColor("#14151b")); dark.setColor(QPalette::Text,Qt::white); dark.setColor(QPalette::WindowText,Qt::white); setPalette(dark);
    setStyleSheet("QMenu { background: transparent; border: none; padding: 7px; margin: 0; color: #ffffff; }"
                  "QMenu::item { background: transparent; padding: 7px 28px 7px 15px; border-radius: 5px; color: #f5f5f7; }"
                  "QMenu::item:selected { background: #522c38; color: white; }"
                  "QMenu::item:disabled { color: #79747e; }"
                  "QMenu::separator { height: 1px; background: #383039; margin: 5px 8px; }"
                  "QMenu::icon { background: transparent; border: none; }"
                  "QMenu::indicator { width: 12px; height: 12px; margin-left: 2px; }"
                  "QMenu::indicator:checked { background: #cf7786; border-radius: 4px; }"
                  "QMenu::indicator:unchecked { background: transparent; }");
}
void AnimatedMenu::showEvent(QShowEvent* e) {
    QMenu::showEvent(e); animation_.stop();
    while (animation_.animationCount()) delete animation_.takeAnimation(0);
    if (qEnvironmentVariableIntValue("TERLOX_REDUCED_MOTION")) { setWindowOpacity(1); return; }
    auto* fade=new QPropertyAnimation(this,"windowOpacity"); fade->setDuration(135); fade->setStartValue(.0); fade->setEndValue(1.); fade->setEasingCurve(QEasingCurve::OutCubic); animation_.addAnimation(fade);
    auto* slide=new QPropertyAnimation(this,"pos"); slide->setDuration(150); slide->setStartValue(pos()+QPoint(0,5)); slide->setEndValue(pos()); slide->setEasingCurve(QEasingCurve::OutCubic); animation_.addAnimation(slide);
    setWindowOpacity(0); animation_.start();
}
void AnimatedMenu::hideEvent(QHideEvent* e) { animation_.stop(); QMenu::hideEvent(e); }
void AnimatedMenu::paintEvent(QPaintEvent* e) {
    { QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.setPen(QPen(QColor(147,77,94,165),1)); p.setBrush(QColor(21,21,28,249)); p.drawRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5),9,9); }
    QMenu::paintEvent(e);
}
}
