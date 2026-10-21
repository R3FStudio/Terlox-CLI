#include "FrameBorder.h"
#include <QPainter>
#include <cmath>
namespace terlox {
void FrameBorder::paint(QPainter& p,const QSize& size,qreal radius,qreal dpr,const QColor& color,const QColor& background) {
    const qreal stroke=2./dpr,half=stroke/2.;
    p.save(); p.setRenderHint(QPainter::Antialiasing,false);
    p.setPen(QPen(color,stroke,Qt::SolidLine,Qt::FlatCap,Qt::MiterJoin)); p.setBrush(Qt::NoBrush);
    if (!radius) { p.drawRect(QRectF(QPointF(),size).adjusted(half,half,-half,-half)); p.restore(); return; }
    if (corners_[0].isNull() || color_!=color || background_!=background || radius_!=radius || dpr_!=dpr) {
        color_=color; background_=background; radius_=radius; dpr_=dpr;
        const int pixels=int(std::ceil(radius*dpr))+1,samples=8;
        const qreal extent=pixels/dpr;
        auto mask=[&](qreal inset) {
            QImage image(pixels*samples,pixels*samples,QImage::Format_ARGB32_Premultiplied); image.fill(Qt::transparent);
            QPainter raster(&image); raster.scale(dpr*samples,dpr*samples); raster.setRenderHint(QPainter::Antialiasing);
            raster.setPen(Qt::NoPen); raster.setBrush(Qt::white);
            raster.drawRoundedRect(QRectF(0,0,extent*2,extent*2).adjusted(inset,inset,-inset,-inset),radius-inset,radius-inset);
            raster.end(); return image.scaled(pixels,pixels,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        };
        const auto outer=mask(0),inner=mask(stroke);
        corners_[0]=QImage(pixels,pixels,QImage::Format_ARGB32_Premultiplied);
        // Fill and outline occupy disjoint coverage. Compositing separately
        // darkens thin curved pixels with an extra layer of black tint.
        for (int y=0;y<pixels;++y) for (int x=0;x<pixels;++x) {
            const qreal inside=qAlpha(inner.pixel(x,y))/255.;
            const qreal ring=std::max(0.,qAlpha(outer.pixel(x,y))/255.-inside);
            const qreal ink=ring*color.alphaF(),fill=inside*background.alphaF();
            corners_[0].setPixel(x,y,qRgba(qRound(ink*color.red()+fill*background.red()),
                qRound(ink*color.green()+fill*background.green()),qRound(ink*color.blue()+fill*background.blue()),qRound(255*(ink+fill))));
        }
        corners_[0].setDevicePixelRatio(dpr);
        corners_[1]=corners_[0].mirrored(true,false);
        corners_[2]=corners_[0].mirrored(false,true);
        corners_[3]=corners_[0].mirrored(true,true);
    }
    const qreal extent=corners_[0].width()/dpr;
    p.drawLine(QPointF(extent,half),QPointF(size.width()-extent,half));
    p.drawLine(QPointF(extent,size.height()-half),QPointF(size.width()-extent,size.height()-half));
    p.drawLine(QPointF(half,extent),QPointF(half,size.height()-extent));
    p.drawLine(QPointF(size.width()-half,extent),QPointF(size.width()-half,size.height()-extent));
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawImage(QPointF(0,0),corners_[0]); p.drawImage(QPointF(size.width()-extent,0),corners_[1]);
    p.drawImage(QPointF(0,size.height()-extent),corners_[2]); p.drawImage(QPointF(size.width()-extent,size.height()-extent),corners_[3]);
    p.restore();
}
}
