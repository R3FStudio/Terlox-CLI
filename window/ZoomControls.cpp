#include "ZoomControls.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
namespace terlox {
ZoomControls::ZoomControls(QWidget* parent):QWidget(parent) {
    setObjectName("terminalZoomControls");
    auto* layout=new QHBoxLayout(this); layout->setContentsMargins(13,7,10,7); layout->setSpacing(7);
    auto* title=new QLabel("Zoom",this); layout->addWidget(title); layout->addStretch();
    auto button=[&](const QString& text,const QString& name,const QString& description) {
        auto* control=new QToolButton(this); control->setText(text); control->setObjectName(name);
        control->setAccessibleName(description); control->setToolTip(description);
        control->setFocusPolicy(Qt::StrongFocus); control->setFixedSize(29,29);
        layout->addWidget(control); return control;
    };
    minus_=button(QString(QChar(0x2212)),"zoomOut","Zoom out");
    value_=new QLabel(this); value_->setObjectName("zoomValue"); value_->setAlignment(Qt::AlignCenter);
    value_->setMinimumWidth(53); layout->addWidget(value_);
    plus_=button("+","zoomIn","Zoom in");
    reset_=button(QString(QChar(0x21ba)),"zoomReset","Reset zoom");
    setStyleSheet("QWidget#terminalZoomControls { background:transparent; }"
        "QLabel { background:transparent; color:#f5f5f7; font-size:12px; }"
        "QToolButton { color:#f5f5f7; background:#27212b; border:1px solid #4b3540; border-radius:6px; font-size:18px; }"
        "QToolButton:hover, QToolButton:focus { background:#522c38; border-color:#ba5666; }"
        "QToolButton:pressed { background:#74394b; } QToolButton:disabled { color:#79747e; }");
    connect(minus_,&QToolButton::clicked,this,[this] { emit stepRequested(-1); });
    connect(plus_,&QToolButton::clicked,this,[this] { emit stepRequested(1); });
    connect(reset_,&QToolButton::clicked,this,&ZoomControls::resetRequested);
}
void ZoomControls::setZoom(qreal points,qreal baseline) {
    value_->setText(QString::number(qRound(points/baseline*100))+"%");
    value_->setToolTip(QString("%1 pt; default %2 pt").arg(points).arg(baseline));
    value_->setAccessibleName(QString("Zoom %1 percent, %2 point text").arg(qRound(points/baseline*100)).arg(points));
    minus_->setEnabled(points>6); plus_->setEnabled(points<40);
    reset_->setEnabled(!qFuzzyCompare(points,baseline));
}
}
