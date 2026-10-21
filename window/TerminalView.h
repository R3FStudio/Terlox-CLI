#pragma once
#include "terminal/Screen.h"
#include "settings/Appearance.h"
#include <QQuickWidget>
#include "renderer/SceneItem.h"
#include <QTimer>
#include <QVariantAnimation>
#include <memory>
class QScrollBar;
namespace terlox {
class TerminalView : public QQuickWidget {
    Q_OBJECT
public:
    TerminalView(Screen* screen, const Appearance& appearance, QWidget* parent = nullptr);
    void setAppearance(const Appearance& appearance);
    void copy(); void paste(); void selectAll();
    QString selectedText() const;
    Screen* screen() const { return screen_; }
    QString rendererDescription() const;
    void applyApplicationControls(const QJsonObject& controls);
    qreal backgroundOpacity() const { return appearance_.opacity; }
    const Appearance& effectiveAppearance() const { return appearance_; }
    void openAbout(const QString& report);
    bool aboutActive() const { return bool(aboutScreen_); }
signals:
    void gridChanged(int rows, int columns);
    void menuRequested(const QPoint& globalPosition);
    void informationRequested();
    void applicationCommandRequested(const QString& command);
    void fontZoomRequested(int steps);
    void surfaceAppearanceChanged();
protected:
    bool eventFilter(QObject*,QEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void focusInEvent(QFocusEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;
    void inputMethodEvent(QInputMethodEvent*) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
private:
    void updateGrid();
    void refreshScene();
    void appendProgress(Frame& frame);
    void applyEffectiveAppearance();
    void initializeScrollBar(); void updateScrollBar();
    void scrollTo(qreal lines,bool animated=true); void setScrollPosition(qreal lines); void resetScrollback();
    void closeAbout(); void renderAbout(); bool aboutKey(QKeyEvent* event);
    QPoint cellAt(const QPointF& position) const;
    bool selected(int row, int col) const;
    void deleteSelectedInput();
    Screen* screen_; Appearance appearance_;
    Appearance baseAppearance_; QJsonObject appControls_;
    Screen* processScreen_=nullptr; std::unique_ptr<Screen> aboutScreen_;
    QString aboutReport_; int aboutPage_=0, aboutScroll_=0;
    SceneItem* scene_;
    QScrollBar* scrollBar_=nullptr;
    QVariantAnimation scrollAnimation_;
    QVariantAnimation progressAnimation_;
    qreal scrollPosition_=0,scrollTarget_=0;
    qint64 lastScrollSerial_=0;
    static constexpr int ScrollGutter=14;
    QTimer refresh_;
    QString typedCommand_; bool commandTracking_ = true;
    qreal cellWidth_ = 8, cellHeight_ = 16, ascent_ = 12;
    QTimer blink_; bool cursorOn_ = true, textBlinkOn_ = true, dragging_ = false, hasSelection_ = false;
    int zoomWheelDelta_=0;
    QPoint anchor_, extent_; int offset_ = 0;
};
}
