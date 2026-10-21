#include "IconPainter.h"
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
namespace terlox {
QImage renderBrandIcon(int size) {
    const int canvas=std::max(1024,size);
    QImage image(canvas,canvas,QImage::Format_ARGB32_Premultiplied); image.fill(Qt::transparent);
    QPainter p(&image); p.setRenderHint(QPainter::Antialiasing); p.scale(canvas/1024.,canvas/1024.);

    // A wide, frameless terminal keeps the silhouette close to the app itself.
    const qreal rim=size<=32?32:20;
    p.setPen(Qt::NoPen); p.setBrush(QColor("#ba5666"));
    p.drawRoundedRect(QRectF(48,132,928,760),144,144);
    QLinearGradient glass(160,152,790,872);
    glass.setColorAt(0,QColor("#27272f")); glass.setColorAt(.5,QColor("#121318")); glass.setColorAt(1,QColor("#090b10"));
    QPainterPath surface; surface.addRoundedRect(QRectF(48+rim,132+rim,928-2*rim,760-2*rim),144-rim,144-rim);
    p.fillPath(surface,glass);

    p.save(); p.setClipPath(surface);
    QLinearGradient reflection(180,152,530,540);
    reflection.setColorAt(0,QColor(255,255,255,14)); reflection.setColorAt(1,QColor(255,255,255,0));
    p.fillRect(QRectF(48,132,928,760),reflection); p.restore();

    // Keep both marks bold enough to read in a taskbar or a 16 px file list.
    QPainterPath prompt; prompt.moveTo(274,354); prompt.lineTo(444,512); prompt.lineTo(274,670);
    p.setBrush(Qt::NoBrush); p.setPen(QPen(QColor("#e68294"),76,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    p.drawPath(prompt);
    p.setPen(Qt::NoPen); p.setBrush(QColor("#e68294"));
    p.drawRoundedRect(QRectF(556,632,208,76),18,18);
    p.end();
    return size==canvas?image:image.scaled(size,size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
}
}
