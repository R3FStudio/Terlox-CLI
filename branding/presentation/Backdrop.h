#pragma once
#include <QWidget>
class PresentationBackdrop:public QWidget {
public:
    explicit PresentationBackdrop(QWidget* parent=nullptr);
protected:
    void paintEvent(QPaintEvent*) override;
};
