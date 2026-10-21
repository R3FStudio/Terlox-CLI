#include "TerminalView.h"
#include <algorithm>
namespace terlox {
void TerminalView::appendProgress(Frame& frame) {
    const auto progress=screen_->progress();
    if (progress.state!=3 && progressAnimation_.state()==QAbstractAnimation::Running) progressAnimation_.stop();
    if (!progress.state) return;
    if (progress.state==3 && progressAnimation_.state()!=QAbstractAnimation::Running) progressAnimation_.start();
    const qreal width=std::min(qreal(180),std::max(qreal(0),frame.size.width()-ScrollGutter-24));
    if (!width) return;
    const QRectF track(frame.size.width()-ScrollGutter-width-8,frame.size.height()-4,width,2);
    frame.fills.append({track,QColor(85,85,95,100)});
    QColor color=progress.state==2?QColor("#ef637a"):progress.state==4?QColor("#d4ab69"):appearance_.border;
    auto fill=track;
    if (progress.state==3) {
        fill.setWidth(width*.25);
        fill.moveLeft(track.left()+(width-fill.width())*progressAnimation_.currentValue().toReal());
    } else fill.setWidth(width*progress.percent/100.);
    frame.fills.append({fill,color});
}
}
