#include "SceneItem.h"
#include "GlyphCache.h"
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGRectangleNode>
#include <cmath>
namespace terlox {
namespace {
struct SceneNode : QSGNode {
    QSGNode fillsLayer,imagesLayer,glyphsLayer;
    QVector<QSGImageNode*> images; quint64 imageRevision=~quint64(0);
    QSGRectangleNode* tint=nullptr; QSGRectangleNode* cursor=nullptr;
    QVector<QSGRectangleNode*> fills; QVector<QSGImageNode*> glyphs;
    GlyphCache cache; QSizeF size; qreal dpr=0;
    SceneNode(QQuickWindow* window) {
        // Stack-allocated layer nodes must not be deleted by the base destructor.
        fillsLayer.setFlag(OwnedByParent,false); imagesLayer.setFlag(OwnedByParent,false); glyphsLayer.setFlag(OwnedByParent,false);
        tint=window->createRectangleNode(); tint->setColor(Qt::transparent); appendChildNode(tint);
        appendChildNode(&fillsLayer); appendChildNode(&imagesLayer); appendChildNode(&glyphsLayer);
        cursor=window->createRectangleNode(); appendChildNode(cursor);
    }
    ~SceneNode() override {
        for (auto* glyph:glyphs) { glyphsLayer.removeChildNode(glyph); delete glyph; }
        for (auto* fill:fills) { fillsLayer.removeChildNode(fill); delete fill; }
        for (auto* image:images) { imagesLayer.removeChildNode(image); delete image; }
        removeChildNode(&imagesLayer);
        removeChildNode(&fillsLayer); removeChildNode(&glyphsLayer);
    }
    void clearGlyphs() { for (auto* glyph:glyphs) { glyphsLayer.removeChildNode(glyph); delete glyph; } glyphs.clear(); cache.clear(); }
};
}
SceneItem::SceneItem(QQuickItem* parent):QQuickItem(parent) { setFlag(ItemHasContents); setClip(true); }
void SceneItem::setFrame(Frame frame) { frame_=std::move(frame); setSize(frame_.size); update(); }
QSGNode* SceneItem::updatePaintNode(QSGNode* previous,UpdatePaintNodeData*) {
    auto* root=static_cast<SceneNode*>(previous); if (!root) root=new SceneNode(window());
    if (root->size!=frame_.size || root->dpr!=frame_.dpr) {
        root->clearGlyphs(); root->size=frame_.size; root->dpr=frame_.dpr;
    }
    if (root->cache.size()>4096) root->clearGlyphs();
    root->tint->setRect(QRectF(QPointF(),frame_.size));
    auto tint=frame_.background; tint.setAlpha(qRound(frame_.backgroundOpacity*255)); root->tint->setColor(tint);
    if (root->imageRevision!=frame_.graphicRevision || root->images.size()!=frame_.graphics.size()) {
        for (auto* image:root->images) { root->imagesLayer.removeChildNode(image); delete image; } root->images.clear();
        for (const auto& graphic:frame_.graphics) { auto* image=window()->createImageNode(); image->setOwnsTexture(true); image->setTexture(window()->createTextureFromImage(graphic.image)); image->setRect(graphic.rect); image->setSourceRect(QRectF(QPointF(),graphic.image.size())); image->setFiltering(QSGTexture::Linear); root->imagesLayer.appendChildNode(image); root->images.append(image); }
        root->imageRevision=frame_.graphicRevision;
    }
    for (qsizetype i=0;i<root->images.size();++i) root->images[i]->setRect(frame_.graphics[i].rect);
    while (root->fills.size()>frame_.fills.size()) { auto* n=root->fills.takeLast(); root->fillsLayer.removeChildNode(n); delete n; }
    for (qsizetype i=0;i<frame_.fills.size();++i) {
        if (i==root->fills.size()) { auto* n=window()->createRectangleNode(); n->setRect(frame_.fills[i].rect); n->setColor(frame_.fills[i].color); root->fills << n; root->fillsLayer.appendChildNode(n); }
        root->fills[i]->setRect(frame_.fills[i].rect); root->fills[i]->setColor(frame_.fills[i].color);
    }
    while (root->glyphs.size()>frame_.glyphs.size()) { auto* n=root->glyphs.takeLast(); root->glyphsLayer.removeChildNode(n); delete n; }
    for (qsizetype i=0;i<frame_.glyphs.size();++i) {
        const auto padding=std::ceil(2*frame_.dpr)/frame_.dpr;
        const auto& g=frame_.glyphs[i]; auto* texture=root->cache.texture(window(),g,frame_.dpr);
        if (i==root->glyphs.size()) {
            auto* n=window()->createImageNode(); n->setFiltering(QSGTexture::Linear);
            // The software scene graph inspects nodes immediately on attachment.
            n->setTexture(texture); n->setSourceRect(QRectF(QPointF(),texture->textureSize())); n->setRect(g.rect.adjusted(-padding,-padding,padding,padding));
            root->glyphs << n; root->glyphsLayer.appendChildNode(n);
        }
        root->glyphs[i]->setTexture(texture); root->glyphs[i]->setSourceRect(QRectF(QPointF(),texture->textureSize()));
        root->glyphs[i]->setRect(g.rect.adjusted(-padding,-padding,padding,padding));
    }
    root->cursor->setRect(frame_.cursor); root->cursor->setColor(QColor(255,255,255,170));
    return root;
}
}
