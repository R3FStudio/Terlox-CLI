#include "TerminalView.h"
#include "renderer/GraphicsBackend.h"
#include <QFontMetricsF>
#include <QResizeEvent>
#include <cmath>
namespace terlox {
static QWidget* prepareGraphics(QWidget* parent) { configureGraphics(); return parent; }
TerminalView::TerminalView(Screen* screen, const Appearance& a, QWidget* parent) : QQuickWidget(prepareGraphics(parent)), screen_(screen), scene_(new SceneItem) {
    auto surfaceFormat=format(); surfaceFormat.setAlphaBufferSize(8); setFormat(surfaceFormat);
    setClearColor(Qt::transparent); setResizeMode(QQuickWidget::SizeRootObjectToView);
    setContent(QUrl(),nullptr,scene_);
    setFocusPolicy(Qt::StrongFocus); setAttribute(Qt::WA_InputMethodEnabled); setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true); setCursor(Qt::IBeamCursor);
    initializeScrollBar();
    setAccessibleName("Terminal"); setAccessibleDescription("Terminal session. Control A selects all; Control C copies selected text; Control V pastes. Drag to select text. The blank top strip moves the window. Shift and drag overrides application mouse input. Click focuses typing.");
    refresh_.setSingleShot(true); refresh_.setInterval(8);
    progressAnimation_.setStartValue(0.); progressAnimation_.setEndValue(1.);
    progressAnimation_.setDuration(1200); progressAnimation_.setLoopCount(-1);
    connect(&progressAnimation_,&QVariantAnimation::valueChanged,this,[this] { refreshScene(); });
    connect(&refresh_,&QTimer::timeout,this,&TerminalView::refreshScene);
    connect(screen_, &Screen::changed, this, [this] { if (!refresh_.isActive()) refresh_.start(); });
    blink_.setInterval(550); connect(&blink_, &QTimer::timeout, this, [this] {
        textBlinkOn_=!textBlinkOn_;
        if (hasFocus() && screen_->cursorVisible() && screen_->cursorBlinks() && !offset_) cursorOn_=!cursorOn_;
        refreshScene();
    }); blink_.start();
    connect(screen_,&Screen::applicationControls,this,&TerminalView::applyApplicationControls,Qt::QueuedConnection);
    setAppearance(a);
}
void TerminalView::applyEffectiveAppearance() { auto a=baseAppearance_;
    if (appControls_.contains("opacity")) a.opacity=appControls_["opacity"].toDouble();
    if (appControls_.contains("blur")) a.blur=appControls_["blur"].toDouble();
    if (appControls_.contains("fontFamily")) a.font.setFamily(appControls_["fontFamily"].toString());
    if (appControls_.contains("borderColor")) a.border=QColor(appControls_["borderColor"].toString());
    if (appControls_.contains("scale")) a.font.setPointSizeF(std::clamp(a.font.pointSizeF()*appControls_["scale"].toDouble(),6.,40.)); appearance_ = a; setFont(a.font); QFontMetricsF m(font());
    const auto dpr=devicePixelRatioF();
    // Use the real font advance and whole device pixels. Fractional cell origins
    // resample glyph textures and make neighboring letters look smeared.
    cellWidth_=std::ceil(m.horizontalAdvance('0')*dpr)/dpr;
    cellHeight_=std::ceil(m.height()*dpr)/dpr; ascent_=std::round(m.ascent()*dpr)/dpr;
    updateGrid(); refreshScene(); }
void TerminalView::updateGrid() { const int rows=std::max(1,int(height()/cellHeight_)),columns=std::max(1,int(std::max(1,width()-ScrollGutter)/cellWidth_)); emit gridChanged(rows,columns); if (aboutScreen_) { aboutScreen_->resize(rows,columns); renderAbout(); } }
void TerminalView::resizeEvent(QResizeEvent* e) { QQuickWidget::resizeEvent(e); hasSelection_ = false; updateGrid(); refreshScene(); }
void TerminalView::focusInEvent(QFocusEvent* e) { QQuickWidget::focusInEvent(e); screen_->focus(true); cursorOn_=true; refreshScene(); }
void TerminalView::focusOutEvent(QFocusEvent* e) { QQuickWidget::focusOutEvent(e); screen_->focus(false); refreshScene(); }
QString TerminalView::rendererDescription() const { return graphicsDescription(quickWindow()); }
}
