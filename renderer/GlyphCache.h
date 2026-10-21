#pragma once
#include "Frame.h"
#include <QHash>
#include <QSGTexture>
class QQuickWindow;
namespace terlox {
class GlyphCache {
public:
    ~GlyphCache();
    QSGTexture* texture(QQuickWindow* window,const Glyph& glyph,qreal dpr);
    int size() const { return textures_.size(); }
    void clear();
private:
    QHash<QString,QSGTexture*> textures_;
};
}
