#include "GlyphCache.h"
#include <QQuickWindow>
#include <QPainter>
#include <QFontMetricsF>
#include <QPainterPath>
#include <cmath>
namespace terlox {
GlyphCache::~GlyphCache() { clear(); }
void GlyphCache::clear() { qDeleteAll(textures_); textures_.clear(); }
QSGTexture* GlyphCache::texture(QQuickWindow* window,const Glyph& g,qreal dpr) {
    const QString key=g.font.toString()+'\n'+g.text+'\n'+QString::number(g.color.rgba())+'\n'+QString::number(g.underline)+':'+QString::number(g.ascent)+'\n'+QString::number(g.rect.width())+'x'+QString::number(g.rect.height())+'@'+QString::number(dpr);
    if (auto* texture=textures_.value(key)) return texture;
    const auto padding=std::ceil(2*dpr)/dpr;
    QImage image(QSize(int(std::round((g.rect.width()+2*padding)*dpr)),int(std::round((g.rect.height()+2*padding)*dpr))),QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr); image.fill(Qt::transparent);
    QPainter p(&image); p.setRenderHint(QPainter::TextAntialiasing); p.setFont(g.font); p.setPen(g.color);
    // Custom proportional fonts may exceed a cell; the bundled monospace font
    // uses its native advance. Keep unusually wide fallback glyphs contained.
    const qreal advance=QFontMetricsF(g.font).horizontalAdvance(g.text);
    p.translate(padding,padding);
    p.save();
    if (advance>g.rect.width()) p.scale(g.rect.width()/advance,1);
    p.drawText(QPointF(0,g.ascent),g.text); p.restore();
    if (g.underline) {
        const qreal stroke=std::max(1./dpr,g.rect.height()/18.);
        const qreal y=std::min(g.rect.height()-3*stroke,g.ascent+stroke);
        p.setPen(QPen(g.color,stroke));
        if (g.underline==3) {
            p.setRenderHint(QPainter::Antialiasing);
            QPainterPath wave; wave.moveTo(0,y);
            for (qreal x=0;x<g.rect.width();x+=1./dpr)
                wave.lineTo(x,y+stroke*std::sin(x*3.141592653589793/(2*stroke)));
            p.drawPath(wave);
        } else {
            p.drawLine(QPointF(0,y),QPointF(g.rect.width(),y));
            if (g.underline==2) p.drawLine(QPointF(0,y+2*stroke),QPointF(g.rect.width(),y+2*stroke));
        }
    }
    p.end();
    auto* texture=window->createTextureFromImage(image,QQuickWindow::TextureCanUseAtlas);
    textures_.insert(key,texture); return texture;
}
}
