#include "TerminalView.h"
#include <QApplication>
#include <QClipboard>
#include <algorithm>
#include <cmath>
namespace terlox {
QPoint TerminalView::cellAt(const QPointF& p) const { return {std::clamp(int(p.x() / cellWidth_), 0, screen_->columns()-1), std::clamp(int(std::floor(p.y() / cellHeight_-(scrollPosition_-offset_))), 0, screen_->rows()-1) - offset_}; }
static bool before(const QPoint& a, const QPoint& b) { return a.y() < b.y() || (a.y() == b.y() && a.x() < b.x()); }
bool TerminalView::selected(int row, int col) const { if (!hasSelection_) return false; auto a = anchor_, b = extent_; if (before(b,a)) std::swap(a,b); return !before({col,row}, a) && !before(b, {col,row}); }
QString TerminalView::selectedText() const {
    if (!hasSelection_) return {};
    auto a = anchor_, b = extent_; if (before(b,a)) std::swap(a,b);
    QStringList lines;
    for (int row = a.y(); row <= b.y(); ++row) lines << screen_->text(row + offset_, row == a.y() ? a.x() : 0, row == b.y() ? b.x() : screen_->columns()-1, offset_);
    return lines.join('\n');
}
void TerminalView::copy() { const auto text = selectedText(); if (!text.isEmpty()) QApplication::clipboard()->setText(text); }
void TerminalView::selectAll() {
    anchor_={0,screen_->alternate()?0:-screen_->historySize()};
    int last=screen_->rows()-1;
    while (last>0 && screen_->text(last,0,screen_->columns()-1).isEmpty()) --last;
    extent_={screen_->columns()-1,last}; hasSelection_=true; refreshScene();
}
void TerminalView::deleteSelectedInput() {
    const auto deletion=screen_->alternate()?std::optional<InputDeletion>{}:screen_->editableInput().selectedRange(anchor_,extent_,screen_->columns(),screen_->scrollSerial());
    hasSelection_=false;
    if (deletion) {
        resetScrollback();
        QKeyEvent home(QEvent::KeyPress,Qt::Key_Home,Qt::NoModifier); screen_->key(&home);
        QKeyEvent right(QEvent::KeyPress,Qt::Key_Right,Qt::NoModifier);
        for (int i=0;i<deletion->first;++i) screen_->key(&right);
        QKeyEvent remove(QEvent::KeyPress,Qt::Key_Delete,Qt::NoModifier);
        for (int i=0;i<deletion->count;++i) screen_->key(&remove);
        typedCommand_=deletion->remaining; commandTracking_=deletion->atEnd;
        cursorOn_=true; blink_.start();
    }
    refreshScene();
}
void TerminalView::paste() { if (aboutScreen_) return; hasSelection_ = false; resetScrollback(); const auto text=QApplication::clipboard()->text(); if (commandTracking_ && !text.contains('\n') && !text.contains('\r')) typedCommand_+=text; else { typedCommand_.clear(); commandTracking_=false; } screen_->paste(text); refreshScene(); }
}
