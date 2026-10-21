#pragma once
#include <QMenu>
#include <QParallelAnimationGroup>
namespace terlox {
class AnimatedMenu : public QMenu {
    Q_OBJECT
public:
    explicit AnimatedMenu(QWidget* parent=nullptr);
    AnimatedMenu(const QString& title,QWidget* parent);
protected:
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;
    void paintEvent(QPaintEvent*) override;
private:
    QParallelAnimationGroup animation_;
};
}
