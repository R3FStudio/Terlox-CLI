#include "Backdrop.h"
#include <QPainter>
#include <QPainterPath>
PresentationBackdrop::PresentationBackdrop(QWidget* parent):QWidget(parent) {
    setWindowFlags(Qt::Window|Qt::FramelessWindowHint); setWindowTitle("Terlox screenshot backdrop");
}
void PresentationBackdrop::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.scale(width()/1440.,height()/900.);
    QLinearGradient base(0,0,1440,900); base.setColorAt(0,QColor("#212630")); base.setColorAt(.55,QColor("#14191f")); base.setColorAt(1,QColor("#30232b")); p.fillRect(0,0,1440,900,base);
    QRadialGradient glow(1250,160,920); glow.setColorAt(0,QColor("#6e8c95")); glow.setColorAt(.5,QColor("#30414e")); glow.setColorAt(1,QColor(20,25,31,0)); p.fillRect(0,0,1440,900,glow);
    for (int i=0;i<7;++i) {
        const qreal y=450+i*58;
        QPainterPath ribbon; ribbon.moveTo(-100,y+300); ribbon.cubicTo(300,y-210,760,y+300,1540,y-380);
        ribbon.lineTo(1540,1000); ribbon.lineTo(-100,1000); ribbon.closeSubpath();
        QLinearGradient shade(100,y-20,1000,y+250); shade.setColorAt(0,QColor(112+i*8,74+i*4,86+i*4)); shade.setColorAt(.16,QColor(52+i*4,40+i*2,49+i*2)); shade.setColorAt(1,QColor("#171a21"));
        p.setPen(QPen(QColor(225,173,183,30),1)); p.setBrush(shade); p.drawPath(ribbon);
    }
    QRadialGradient vignette(720,450,860); vignette.setColorAt(0,Qt::transparent); vignette.setColorAt(.6,QColor(0,0,0,0)); vignette.setColorAt(1,QColor(0,0,0,110)); p.fillRect(0,0,1440,900,vignette);
}
