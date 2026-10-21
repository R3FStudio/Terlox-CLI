#include "Screen.h"
#include <algorithm>
namespace terlox {
Screen::Screen(int rows, int columns, int scrollback, QObject* parent)
    : QObject(parent), vt_(vterm_new(rows, columns)), screen_(vterm_obtain_screen(vt_)), history_(scrollback), rows_(rows), columns_(columns) {
    vterm_set_utf8(vt_, 1); vterm_screen_enable_altscreen(screen_, 1);
    vterm_screen_set_damage_merge(screen_, VTERM_DAMAGE_SCREEN);
    static const VTermScreenCallbacks callbacks = {
        [](VTermRect, void* u) { static_cast<Screen*>(u)->markChanged(); return 1; },
        nullptr,
        [](VTermPos, VTermPos, int visible, void* u) { auto* s = static_cast<Screen*>(u); s->visible_ = visible; s->markChanged(); return 1; },
        [](VTermProp p, VTermValue* v, void* u) {
            auto* s = static_cast<Screen*>(u);
            if (p == VTERM_PROP_CURSORVISIBLE) s->visible_ = v->boolean;
            if (p == VTERM_PROP_ALTSCREEN) s->alternate_ = v->boolean;
            if (p == VTERM_PROP_CURSORBLINK) s->cursorBlinks_=v->boolean;
            if (p == VTERM_PROP_CURSORSHAPE) s->cursorShape_=v->number;
            if (p == VTERM_PROP_MOUSE) s->mouseMode_=v->number;
            s->markChanged(); return 1;
        },
        nullptr, nullptr,
        [](int c, const VTermScreenCell* cells, void* u) { auto* s=static_cast<Screen*>(u); s->history_.push(c, cells); ++s->scrollSerial_; return 1; },
        [](int c, VTermScreenCell* cells, void* u) { return static_cast<Screen*>(u)->history_.pop(c, cells) ? 1 : 0; },
        [](void* u) { static_cast<Screen*>(u)->history_.clear(); return 1; }, nullptr
    };
    syncWatchdog_.setSingleShot(true); syncWatchdog_.setInterval(150);
    connect(&syncWatchdog_,&QTimer::timeout,this,[this] { synchronized_=false; markChanged(); });
    vterm_screen_set_callbacks(screen_, &callbacks, this);
    vterm_output_set_callback(vt_, [](const char* bytes, size_t size, void* u) { emit static_cast<Screen*>(u)->output(QByteArray(bytes, qsizetype(size))); }, this);
    VTermColor fg, bg; vterm_color_rgb(&fg, 255, 255, 255); vterm_color_rgb(&bg, 0, 0, 0);
    vterm_screen_set_default_colors(screen_, &fg, &bg); vterm_screen_reset(screen_, 1);
}
Screen::~Screen() { vterm_free(vt_); }
void Screen::feed(const QByteArray& b) {
    feeding_=true;
    sequences_.feed(b,[this](const QByteArray& bytes) { writeVt(bytes); },[this](const QByteArray& sequence) { return extension(sequence); });
    vterm_screen_flush_damage(screen_); feeding_=false;
    if (dirty_ && !synchronized_) { dirty_=false; emit changed(); }
}
void Screen::resize(int rows, int columns) { rows_ = std::max(1, rows); columns_ = std::max(1, columns); vterm_set_size(vt_, rows_, columns_); vterm_screen_flush_damage(screen_); emit changed(); }
VTermPos Screen::cursor() const { VTermPos p; vterm_state_get_cursorpos(vterm_obtain_state(vt_), &p); return p; }
QColor Screen::color(VTermColor c) const { if (VTERM_COLOR_IS_DEFAULT_FG(&c)) return foreground_; if (VTERM_COLOR_IS_DEFAULT_BG(&c)) return background_; if (VTERM_COLOR_IS_INDEXED(&c) && palette_.contains(c.indexed.idx)) return palette_.value(c.indexed.idx); vterm_screen_convert_color_to_rgb(screen_, &c); return QColor(c.rgb.red, c.rgb.green, c.rgb.blue); }
VTermScreenCell Screen::cell(int row, int col, int offset) const {
    VTermScreenCell c{}; if (col < 0 || col >= columns_) return c;
    const int effective = row - (alternate_ ? 0 : offset);
    if (effective < 0) {
        const int index = history_.size() + effective;
        if (index >= 0 && index < history_.size() && col < int(history_.line(index).size())) return history_.line(index)[col];
        return c;
    }
    if (effective < rows_) vterm_screen_get_cell(screen_, {effective, col}, &c);
    return c;
}
QString cellText(const VTermScreenCell& c) {
    if (c.chars[0] == 0xffffffff || c.width == 0) return {};
    if (!c.chars[0]) return " ";
    qsizetype count = 0; while (count < VTERM_MAX_CHARS_PER_CELL && c.chars[count]) ++count;
    return QString::fromUcs4(reinterpret_cast<const char32_t*>(c.chars), count);
}
QString Screen::text(int row, int first, int last, int offset) const {
    QString out; for (int col = first; col <= last; ++col) out += cellText(cell(row, col, offset));
    while (out.endsWith(' ')) out.chop(1); return out;
}
}
