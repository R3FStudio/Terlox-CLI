#include "Screen.h"
namespace terlox {
void Screen::key(QKeyEvent* e) {
    if (e->key()==Qt::Key_Return || e->key()==Qt::Key_Enter || (e->key()==Qt::Key_C && e->modifiers().testFlag(Qt::ControlModifier))) editableInput_.clear();
    int mods = VTERM_MOD_NONE;
    if (e->modifiers() & Qt::ShiftModifier) mods |= VTERM_MOD_SHIFT;
    if (e->modifiers() & Qt::ControlModifier) mods |= VTERM_MOD_CTRL;
    if (e->modifiers() & Qt::AltModifier) mods |= VTERM_MOD_ALT;
    VTermKey key = VTERM_KEY_NONE;
    switch (e->key()) {
    case Qt::Key_Return: case Qt::Key_Enter: key = VTERM_KEY_ENTER; break;
    case Qt::Key_Backspace: key = VTERM_KEY_BACKSPACE; break;
    case Qt::Key_Tab: case Qt::Key_Backtab: key = VTERM_KEY_TAB; break;
    case Qt::Key_Escape: key = VTERM_KEY_ESCAPE; break;
    case Qt::Key_Up: key = VTERM_KEY_UP; break; case Qt::Key_Down: key = VTERM_KEY_DOWN; break;
    case Qt::Key_Left: key = VTERM_KEY_LEFT; break; case Qt::Key_Right: key = VTERM_KEY_RIGHT; break;
    case Qt::Key_Home: key = VTERM_KEY_HOME; break; case Qt::Key_End: key = VTERM_KEY_END; break;
    case Qt::Key_PageUp: key = VTERM_KEY_PAGEUP; break; case Qt::Key_PageDown: key = VTERM_KEY_PAGEDOWN; break;
    case Qt::Key_Insert: key = VTERM_KEY_INS; break; case Qt::Key_Delete: key = VTERM_KEY_DEL; break;
    default: if (e->key() >= Qt::Key_F1 && e->key() <= Qt::Key_F35) key = VTermKey(VTERM_KEY_FUNCTION(e->key() - Qt::Key_F1 + 1)); break;
    }
    if (key != VTERM_KEY_NONE) vterm_keyboard_key(vt_, key, VTermModifier(mods));
    else if ((mods & VTERM_MOD_CTRL) && e->key() >= Qt::Key_A && e->key() <= Qt::Key_Z)
        vterm_keyboard_unichar(vt_, 'a' + e->key() - Qt::Key_A, VTermModifier(mods));
    else for (const auto cp : e->text().toUcs4()) vterm_keyboard_unichar(vt_, cp, VTermModifier(mods & ~VTERM_MOD_SHIFT));
}
void Screen::paste(const QString& text) {
    vterm_keyboard_start_paste(vt_);
    // Send UTF-8 without key interpretation; libvterm brackets this if requested.
    emit output(QString(text).replace("\r\n", "\n").replace('\n', '\r').toUtf8());
    vterm_keyboard_end_paste(vt_);
}
}
