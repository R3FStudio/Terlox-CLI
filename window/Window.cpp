#include "Window.h"
#include "sessions/Session.h"
#include "platform/Backdrop.h"
#include "platform/Elevation.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QTabBar>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWindow>
namespace terlox {
Window::Window(const QString& preferred, QWidget* parent, const Profile* initialProfile) : QWidget(parent), appearance_(store_.appearance()), profiles_(store_.profiles(QApplication::applicationDirPath())), tabs_(new QTabWidget(this)) {
    setWindowTitle("Terlox"); setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint); setAttribute(Qt::WA_TranslucentBackground);
    if (isAdministrator()) setWindowTitle("Terlox — Administrator");
    setMinimumSize(320, 200); resize(900, 600); setMouseTracking(true);
    auto* layout = new QVBoxLayout(this); layout->setContentsMargins(9,TopInset,9,9); layout->setSpacing(0); layout->addWidget(tabs_);
    tabs_->setDocumentMode(true); tabs_->setTabsClosable(true); tabs_->setMovable(true);
    tabs_->setStyleSheet("QTabWidget::pane { border:0; background:transparent; } QTabBar::tab { background:transparent; color:#bbb6c0; padding:5px 12px; } QTabBar::tab:selected { color:white; border-bottom:1px solid #ba5666; } QStackedWidget { background:transparent; }");
    connect(tabs_, &QTabWidget::tabCloseRequested, this, &Window::closeTab);
    connect(tabs_, &QTabWidget::currentChanged, this, [this] {
        setFocusProxy(current()); applySessionSurface(); focusTerminal();
    });
    installShortcuts();
    if (initialProfile) { newSession(*initialProfile); return; }
    const auto selected = preferred.isEmpty() ? store_.defaultProfile() : preferred;
    for (const auto& p : profiles_) if (p.name == selected) { newSession(p); return; }
    if (!profiles_.isEmpty()) newSession(profiles_.first());
}
TerminalView* Window::current() const { return qobject_cast<TerminalView*>(tabs_->currentWidget()); }
void Window::newDefaultSession() { for (const auto& p : profiles_) if (p.name == store_.defaultProfile()) { newSession(p); return; } if (!profiles_.isEmpty()) newSession(profiles_.first()); }
void Window::newSession(const Profile& profile) {
    auto* session = new Session(profile.launch, appearance_.scrollback);
    auto* view = new TerminalView(&session->screen(), displayAppearance()); session->setParent(view);
    connect(view, &TerminalView::gridChanged, session, &Session::resize);
    connect(view,&TerminalView::surfaceAppearanceChanged,this,&Window::applySessionSurface);
    connect(view,&TerminalView::fontZoomRequested,this,&Window::zoomText);
    connect(view, &TerminalView::menuRequested, this, &Window::showMenu);
    connect(view,&TerminalView::informationRequested,this,&Window::showInformation);
    connect(view,&TerminalView::applicationCommandRequested,this,&Window::executeApplicationCommand);
    view->setProperty("profileName",profile.name); view->setProperty("profileExecutable",profile.launch.executable); view->setProperty("profileDirectory",profile.launch.directory);
    view->setProperty("profileArguments",profile.launch.arguments);
    connect(session, &Session::finished, view, [this, view](int code) { const auto index = tabs_->indexOf(view); if (index >= 0) tabs_->setTabText(index, tabs_->tabText(index) + QString(" [%1]").arg(code)); });
    if (profile.launch.inbound) connect(session,&Session::finished,view,[this,view] { const auto index=tabs_->indexOf(view); if (index>=0) closeTab(index); },Qt::QueuedConnection);
    const int index = tabs_->addTab(view, profile.name); tabs_->setCurrentIndex(index); updateTabs(); view->setFocus();
    QTimer::singleShot(0, view, [this,session] { session->start(); focusTerminal(); });
}
void Window::closeTab(int index) { auto* view = tabs_->widget(index); tabs_->removeTab(index); delete view; if (!tabs_->count()) close(); else { updateTabs(); current()->setFocus(); } }
void Window::updateTabs() { tabs_->tabBar()->setVisible(tabs_->count() > 1); applySessionSurface(); }
void Window::applySessionSurface() {
    const auto& a=current()?current()->effectiveAppearance():appearance_;
    nativeBackdrop_=applyBackdrop(winId(),a.blur,a.effects); applyWindowBorder(winId(),frameColor()); update();
}
Appearance Window::displayAppearance() const {
    auto display=appearance_;
    display.font.setPointSizeF(std::clamp(display.font.pointSizeF()+textZoom_,6.,40.));
    return display;
}
void Window::applyAppearance(bool save) { if (save) store_.saveAppearance(appearance_); for (int i = 0; i < tabs_->count(); ++i) static_cast<TerminalView*>(tabs_->widget(i))->setAppearance(displayAppearance()); applySessionSurface(); emit textZoomChanged(displayAppearance().font.pointSizeF(),appearance_.font.pointSizeF()); }
void Window::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    const QRectF panel=QRectF(rect());
    const qreal radius=isMaximized() || isFullScreen()?0.:8.;
    QPainterPath tint; tint.addRoundedRect(panel,radius,radius);
    // The GPU surface supplies its own alpha tint; avoid applying it twice.
    if (auto* view=current(); view && view->isVisible()) {
        QPainterPath content; content.addRect(QRectF(view->mapTo(this,QPoint()),view->size())); tint=tint.subtracted(content);
    }
    auto panelColor=current()?current()->screen()->defaultBackground():QColor(Qt::black); panelColor.setAlpha(qRound(255*(current()?current()->backgroundOpacity():appearance_.opacity)));
    p.setPen(Qt::NoPen); p.setBrush(panelColor); p.drawPath(tint);
    border_.paint(p,size(),radius,devicePixelRatioF(),frameColor(),panelColor);
}
}
