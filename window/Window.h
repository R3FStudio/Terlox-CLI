#pragma once
#include "settings/Store.h"
#include "TerminalView.h"
#include "FrameBorder.h"
#include <QWidget>
#include <QTabWidget>
namespace terlox {
class Window : public QWidget {
    Q_OBJECT
public:
    explicit Window(const QString& profile = {}, QWidget* parent = nullptr, const Profile* initialProfile = nullptr);
    void newSession(const Profile& profile);
    void newDefaultSession();
    void showMenu(const QPoint& position);
    void showInformation();
    QString informationText() const;
    void executeApplicationCommand(const QString& command);
signals:
    void textZoomChanged(qreal points,qreal baseline);
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;
    void showEvent(QShowEvent*) override;
    void changeEvent(QEvent*) override;
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray& type,void* message,qintptr* result) override;
#endif
private:
    TerminalView* current() const;
    void closeTab(int index);
    void updateTabs();
    void applyAppearance(bool save=true);
    void applySessionSurface();
    QColor frameColor() const;
    void focusTerminal();
    Appearance displayAppearance() const;
    void installShortcuts();
    void zoomText(int steps);
    void openAdminSession(const QString& engine={},const QString& command={});
    Qt::Edges edges(const QPointF& position) const;
    bool dragArea(const QPointF& position) const;
    Store store_; Appearance appearance_; QList<Profile> profiles_; QTabWidget* tabs_;
    bool nativeBackdrop_=false;
    bool initialActivationRequested_=false;
    qreal textZoom_=0;
    FrameBorder border_;
    static constexpr int TopInset=32;
};
}
