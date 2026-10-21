#include "Scrollback.h"
#include <algorithm>
namespace terlox {
void Scrollback::push(int columns, const VTermScreenCell* cells) {
    if (limit_ <= 0) return;
    lines_.emplace_back(cells, cells + columns);
    while (size() > limit_) lines_.pop_front();
}
bool Scrollback::pop(int columns, VTermScreenCell* cells) {
    if (lines_.empty()) return false;
    const auto& l = lines_.back(); std::fill(cells, cells + columns, VTermScreenCell{});
    std::copy_n(l.begin(), std::min(int(l.size()), columns), cells); lines_.pop_back(); return true;
}
}
