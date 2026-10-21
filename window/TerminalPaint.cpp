#include "TerminalView.h"
namespace terlox {
void TerminalView::refreshScene() {
    if (screen_->synchronizedUpdates()) return;
    updateScrollBar();
    const qreal shift=(scrollPosition_-offset_)*cellHeight_;
    Frame frame; frame.size=size(); frame.dpr=devicePixelRatioF(); frame.backgroundOpacity=appearance_.opacity; frame.background=screen_->defaultBackground(); frame.graphicRevision=(screen_->graphicRevision()<<32)^quint64(offset_)^(quint64(screen_->scrollSerial())<<8);
    for (const auto& image:screen_->graphics()) {
        const auto row=image.line-screen_->scrollSerial()+offset_;
        const QSizeF extent(image.columns>0?image.columns*cellWidth_:image.image.width(),image.rows>0?image.rows*cellHeight_:image.image.height());
        if (row*cellHeight_+shift+extent.height()<0 || row*cellHeight_+shift>height()) continue;
        frame.graphics.append({image.image,QRectF(QPointF(image.cell.x()*cellWidth_,row*cellHeight_+shift),extent)});
    }
    frame.glyphs.reserve(screen_->rows()*screen_->columns()/3);
    for (int row = 0; row < screen_->rows()+(shift<0?1:0); ++row) {
        for (int col = 0; col < screen_->columns(); ++col) {
            const auto c = screen_->cell(row, col, offset_); if (!c.width || c.chars[0] == 0xffffffff) continue;
            const QRectF rect(col * cellWidth_, row * cellHeight_+shift, cellWidth_ * std::max(1, int(c.width)), cellHeight_);
            QColor fg = screen_->color(c.fg), bg = screen_->color(c.bg);
            fg.setAlpha(c.attrs.foreground_alpha); bg.setAlpha(c.attrs.background_alpha);
            bool defaultBg = VTERM_COLOR_IS_DEFAULT_BG(&c.bg);
            if (c.attrs.reverse) { std::swap(fg, bg); defaultBg = false; }
            if (selected(row - offset_, col)) { bg = QColor(178, 64, 85, 165); defaultBg = false; }
            if (!defaultBg) frame.fills.append({rect,bg});
            if (c.attrs.conceal || (c.attrs.blink && !textBlinkOn_)) continue;
            if (c.attrs.faint) fg.setAlpha(qRound(fg.alpha()*.5));
            auto f = font(); f.setBold(c.attrs.bold); f.setItalic(c.attrs.italic); f.setStrikeOut(c.attrs.strike);
            qreal baseline=ascent_;
            if (c.attrs.small) {
                f.setPointSizeF(f.pointSizeF()*.75);
                if (c.attrs.baseline==VTERM_BASELINE_RAISE) baseline-=cellHeight_*.25;
                else if (c.attrs.baseline==VTERM_BASELINE_LOWER) baseline+=cellHeight_*.1;
            }
            const auto text=cellText(c);
            if (!text.isEmpty() && (text!=" " || c.attrs.underline || c.attrs.strike)) frame.glyphs.append({rect,text,f,fg,baseline,int(c.attrs.underline)});
        }
    }
    if (hasFocus() && cursorOn_ && screen_->cursorVisible() && offset_ == 0) {
        const auto cursor = screen_->cursor();
        frame.cursor=QRectF(cursor.col * cellWidth_, cursor.row * cellHeight_+shift, cellWidth_, cellHeight_);
        if (screen_->cursorShape()==VTERM_PROP_CURSORSHAPE_UNDERLINE) frame.cursor.setTop(frame.cursor.bottom()-2);
        else if (screen_->cursorShape()==VTERM_PROP_CURSORSHAPE_BAR_LEFT) frame.cursor.setWidth(2);
    }
    appendProgress(frame);
    scene_->setFrame(std::move(frame));
}
}
