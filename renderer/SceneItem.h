#pragma once
#include "Frame.h"
#include <QQuickItem>
namespace terlox {
class SceneItem : public QQuickItem {
    Q_OBJECT
public:
    explicit SceneItem(QQuickItem* parent=nullptr);
    void setFrame(Frame frame);
    int glyphCount() const { return int(frame_.glyphs.size()); }
    const Frame& frame() const { return frame_; }
protected:
    QSGNode* updatePaintNode(QSGNode* previous,UpdatePaintNodeData*) override;
private:
    Frame frame_;
};
}
