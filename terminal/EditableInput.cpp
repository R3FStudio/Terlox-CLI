#include "EditableInput.h"
#include <QJsonArray>
#include <algorithm>
#include <cmath>
#include <numeric>
namespace terlox {
void EditableInput::update(const QJsonObject& state,int row,int column,int columns,qint64 scrollSerial) {
    clear();
    if (!state.value("active").toBool() || !state.value("text").isString() || columns<1) return;
    const auto text=state.value("text").toString();
    const auto widths=state.value("widths").toArray();
    const auto cursor=state.value("cursor").toDouble(-1);
    if (text.size()>8192 || text.contains('\n') || text.contains('\r') || widths.size()!=text.toUcs4().size()
        || cursor<0 || cursor>widths.size() || std::floor(cursor)!=cursor) return;
    QVector<int> cells; int prefix=0;
    for (int i=0;i<widths.size();++i) {
        const auto width=widths[i].toDouble(-1);
        if (width<0 || width>2 || std::floor(width)!=width) return;
        cells.append(int(width)); if (i<int(cursor)) prefix+=int(width);
    }
    columns_=columns; origin_=(scrollSerial+row)*columns+column-prefix;
    text_=text; widths_=std::move(cells); active_=true;
}
std::optional<InputDeletion> EditableInput::selectedRange(QPoint a,QPoint b,int columns,qint64 scrollSerial) const {
    if (!active_ || columns!=columns_) return {};
    auto absolute=[&](QPoint p) { return (scrollSerial+p.y())*columns+p.x(); };
    auto left=absolute(a),right=absolute(b); if (right<left) std::swap(left,right);
    const auto total=std::accumulate(widths_.begin(),widths_.end(),qint64(0));
    if (!widths_.isEmpty() && left<=origin_ && right>=origin_+total-1)
        return InputDeletion{0,int(widths_.size()),{},true};
    int first=-1,last=-1; qint64 position=origin_,base=origin_; int baseWidth=1;
    for (int i=0;i<widths_.size();++i) {
        const int width=widths_[i];
        // Leave uncertain wide-character wraps to the shell's own editing keys.
        if (width==2 && position%columns==columns-1) return {};
        if (width) { base=position; baseWidth=width; }
        if (left<=base+baseWidth-1 && right>=base) { if (first<0) first=i; last=i; }
        position+=width;
    }
    if (first<0) return {};
    auto points=text_.toUcs4();
    const int count=last-first+1;
    points.remove(first,count);
    return InputDeletion{first,count,QString::fromUcs4(reinterpret_cast<const char32_t*>(points.constData()),points.size()),first==points.size()};
}
}
