#include "Screen.h"
#include <QJsonDocument>
#include <cmath>
namespace terlox {
namespace {
QColor parseColor(QByteArray value) {
    if (!value.startsWith("rgb:")) return QColor(QString::fromLatin1(value));
    const auto parts=value.mid(4).split('/'); if (parts.size()!=3) return {};
    int channels[3]; for (int i=0;i<3;++i) { bool valid; const auto n=parts[i].toUInt(&valid,16); if (!valid || parts[i].isEmpty() || parts[i].size()>4) return {}; channels[i]=int(n*255/((1u<<(parts[i].size()*4))-1)); }
    return QColor(channels[0],channels[1],channels[2]);
}
}
bool Screen::themeSequence(int command,const QByteArray& data) {
    auto* state=vterm_obtain_state(vt_);
    auto reply=[&](int cmd,const QColor& color,const QByteArray& prefix=QByteArray()) { emit output("\033]"+QByteArray::number(cmd)+';'+prefix+"rgb:"+QByteArray::number(color.red()*257,16).rightJustified(4,'0')+'/'+QByteArray::number(color.green()*257,16).rightJustified(4,'0')+'/'+QByteArray::number(color.blue()*257,16).rightJustified(4,'0')+"\033\\"); };
    if (command==104) { if (data.isEmpty()) palette_.clear(); else for (const auto& field:data.split(';')) { bool valid; const int index=field.toInt(&valid); if (valid) palette_.remove(index); } markChanged(); return true; }
    if (command==4) {
        const auto fields=data.split(';');
        for (int i=0;i+1<fields.size();i+=2) { bool valid; const auto index=fields[i].toInt(&valid); if (!valid || index<0 || index>255) continue;
            if (fields[i+1]=="?") { VTermColor color; vterm_state_get_palette_color(state,index,&color); reply(4,palette_.value(index,this->color(color)),fields[i]+';'); }
            else { const auto color=parseColor(fields[i+1]); if (!color.isValid()) continue; palette_[index]=color; }
        }
        markChanged(); return true;
    }
    if (command!=10 && command!=11 && command!=110 && command!=111) return false;
    if (data=="?") { reply(command,command==10?foreground_:background_); return true; }
    const auto color=command==110?QColor(Qt::white):command==111?QColor(Qt::black):parseColor(data);
    if (!color.isValid()) return true;
    if (command==10 || command==110) foreground_=color; else { background_=color; themedBackground_=command!=111; }
    VTermColor fg,bg; vterm_color_rgb(&fg,foreground_.red(),foreground_.green(),foreground_.blue()); vterm_color_rgb(&bg,background_.red(),background_.green(),background_.blue());
    vterm_screen_set_default_colors(screen_,&fg,&bg); markChanged(); return true;
}
bool Screen::extension(const QByteArray& sequence) {
    if (sequence=="\033[?2026h") { synchronized_=true; syncWatchdog_.start(); return true; }
    if (sequence=="\033[?2026l") { synchronized_=false; syncWatchdog_.stop(); markChanged(); return true; }
    if (sequence=="\033[?2026$p") { emit output(QByteArray("\033[?2026;")+(synchronized_?'1':'2')+"$y"); return true; }
    if (sequence=="\033[?1049h" || sequence=="\033[?1049l") editableInput_.clear();
    if (sequence=="\033[2J" || sequence=="\033[3J" || sequence=="\033[?1049l") { graphics_.clear(); if (sequence.endsWith("1049l")) emit applicationControls(QJsonObject{{"reset",true}}); }
    if (sequence.startsWith("\033_G")) {
        const auto pos=cursor(); const bool handled=graphics_.kitty(sequence.mid(2,sequence.size()-4),{pos.col,pos.row},scrollSerial_,[this](const QByteArray& reply) { emit output(reply); });
        if (handled) markChanged(); return handled;
    }
    if (!sequence.startsWith("\033]")) return false;
    const auto end=sequence.endsWith('\a')?1:2; const auto payload=sequence.mid(2,sequence.size()-2-end);
    const auto split=payload.indexOf(';'); bool valid; const auto command=payload.left(split<0?payload.size():split).toInt(&valid); if (!valid) return false;
    const auto data=split<0?QByteArray():payload.mid(split+1);
    if (command==9 && progress_.parse(data)) { markChanged(); return true; }
    if (themeSequence(command,data)) return true;
    if (command==777 && data.startsWith("terlox-input;")) {
        editableInput_.clear();
        const auto json=QByteArray::fromBase64(data.mid(13));
        if (json.size()<=65536) {
            const auto doc=QJsonDocument::fromJson(json);
            if (doc.isObject()) { const auto pos=cursor(); editableInput_.update(doc.object(),pos.row,pos.col,columns_,scrollSerial_); }
        }
        markChanged(); return true;
    }
    if (command==777 && data.startsWith("terlox;")) {
        const auto json=QByteArray::fromBase64(data.mid(7));
        if (json.size()<=4096) {
            const auto doc=QJsonDocument::fromJson(json);
            if (doc.isObject()) {
                const auto controls=doc.object();
                auto alpha=[&](const char* key) {
                    const auto value=controls.value(key);
                    if (!value.isDouble()) return -1;
                    const auto n=value.toDouble();
                    return std::isfinite(n) && n>=0 && n<=1?qRound(n*255):-1;
                };
                auto* state=vterm_obtain_state(vt_);
                if (controls.value("reset").toBool()) vterm_state_set_pen_alpha(state,255,255);
                vterm_state_set_pen_alpha(state,alpha("textOpacity"),alpha("cellBackgroundOpacity"));
                emit applicationControls(controls);
            }
        }
        return true;
    }
    if (command==1337) { const auto pos=cursor(); if (graphics_.inlineImage(data,{pos.col,pos.row},scrollSerial_)) { markChanged(); return true; } }
    return false;
}
void Screen::markChanged() { dirty_=true; if (!feeding_ && !synchronized_) { dirty_=false; emit changed(); } }
void Screen::writeVt(const QByteArray& bytes) { vterm_input_write(vt_,bytes.constData(),size_t(bytes.size())); }
void Screen::mouse(const QPoint& cell,int button,bool pressed,Qt::KeyboardModifiers modifiers) {
    int mods=0; if (modifiers.testFlag(Qt::ShiftModifier)) mods|=VTERM_MOD_SHIFT; if (modifiers.testFlag(Qt::ControlModifier)) mods|=VTERM_MOD_CTRL; if (modifiers.testFlag(Qt::AltModifier)) mods|=VTERM_MOD_ALT;
    vterm_mouse_move(vt_,cell.y(),cell.x(),VTermModifier(mods)); vterm_mouse_button(vt_,button,pressed,VTermModifier(mods));
}
void Screen::mouseMove(const QPoint& cell,Qt::KeyboardModifiers modifiers) { vterm_mouse_move(vt_,cell.y(),cell.x(),VTermModifier(modifiers.testFlag(Qt::ControlModifier)?VTERM_MOD_CTRL:VTERM_MOD_NONE)); }
void Screen::focus(bool focused) { if (focused) vterm_state_focus_in(vterm_obtain_state(vt_)); else vterm_state_focus_out(vterm_obtain_state(vt_)); }
}
