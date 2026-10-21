#pragma once
#include <QWidget>
class QLabel;
class QToolButton;
namespace terlox {
class ZoomControls : public QWidget {
    Q_OBJECT
public:
    explicit ZoomControls(QWidget* parent=nullptr);
    void setZoom(qreal points,qreal baseline);
signals:
    void stepRequested(int steps);
    void resetRequested();
private:
    QLabel* value_;
    QToolButton *minus_,*plus_,*reset_;
};
}
