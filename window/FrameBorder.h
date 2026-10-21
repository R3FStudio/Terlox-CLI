#pragma once
#include <QColor>
#include <QImage>
class QPainter;
namespace terlox {
// Cache only the small corner tiles; resizing never rasterizes a full window.
class FrameBorder {
public:
    void paint(QPainter& painter,const QSize& size,qreal radius,qreal dpr,const QColor& color,const QColor& background=Qt::transparent);
private:
    QImage corners_[4];
    QColor color_,background_; qreal radius_=0,dpr_=0;
};
}
