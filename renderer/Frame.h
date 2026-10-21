#pragma once
#include <QColor>
#include <QFont>
#include <QRectF>
#include <QVector>
#include <QImage>
namespace terlox {
struct Glyph { QRectF rect; QString text; QFont font; QColor color; qreal ascent; int underline=0; };
struct CellFill { QRectF rect; QColor color; };
struct GraphicFrame { QImage image; QRectF rect; };
struct Frame { QVector<GraphicFrame> graphics; quint64 graphicRevision=0; QColor background{Qt::black}; QVector<Glyph> glyphs; QVector<CellFill> fills; QRectF cursor; qreal dpr = 1, backgroundOpacity = .68; QSizeF size; };
}
