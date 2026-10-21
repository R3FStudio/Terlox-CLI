#include "TerminalView.h"
#include "information/About.h"
namespace terlox {
void TerminalView::openAbout(const QString& report) {
    if (!aboutScreen_) {
        processScreen_=screen_; aboutScreen_=std::make_unique<Screen>(screen_->rows(),screen_->columns(),0);
        connect(aboutScreen_.get(),&Screen::changed,this,[this] { if (!refresh_.isActive()) refresh_.start(); });
        screen_=aboutScreen_.get();
    }
    aboutReport_=report; aboutPage_=aboutScroll_=0; resetScrollback(); hasSelection_=false;
    renderAbout(); setFocus();
}
void TerminalView::closeAbout() {
    if (!aboutScreen_) return;
    screen_=processScreen_; aboutScreen_.reset(); aboutReport_.clear();
    typedCommand_.clear(); commandTracking_=true; resetScrollback(); hasSelection_=false; setFocus(); refreshScene();
}
void TerminalView::renderAbout() { if (aboutScreen_) aboutScreen_->feed(aboutPage(screen_->columns(),screen_->rows(),aboutPage_,aboutScroll_,aboutReport_,&aboutScroll_)); }
bool TerminalView::aboutKey(QKeyEvent* event) {
    if (!aboutScreen_) return false;
    const int key=event->key();
    if (key==Qt::Key_Escape || key==Qt::Key_Q || key==Qt::Key_F1 || (key==Qt::Key_C && event->modifiers()==Qt::ControlModifier)) { closeAbout(); return true; }
    if (key==Qt::Key_C && event->modifiers()==(Qt::ControlModifier|Qt::ShiftModifier)) return false;
    if (key>=Qt::Key_1 && key<=Qt::Key_4) { aboutPage_=key-Qt::Key_1; aboutScroll_=0; }
    else if (key==Qt::Key_Tab || key==Qt::Key_Right) { aboutPage_=(aboutPage_+1)%4; aboutScroll_=0; }
    else if (key==Qt::Key_Backtab || key==Qt::Key_Left) { aboutPage_=(aboutPage_+3)%4; aboutScroll_=0; }
    else if (key==Qt::Key_Down || key==Qt::Key_PageDown) aboutScroll_+=key==Qt::Key_Down?1:std::max(1,screen_->rows()-8);
    else if (key==Qt::Key_Up || key==Qt::Key_PageUp) aboutScroll_=std::max(0,aboutScroll_-(key==Qt::Key_Up?1:std::max(1,screen_->rows()-8)));
    renderAbout(); return true;
}
}
