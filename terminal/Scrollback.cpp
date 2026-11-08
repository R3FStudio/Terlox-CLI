#include "Scrollback.h"
#include <algorithm>
namespace terlox {
void Scrollback::push(int columns, const VTermScreenCell* cells) {
    if (limit_ <= 0 || columns<=0) return;
    const auto bytes=size_t(columns)*sizeof(VTermScreenCell);
    if (bytes>byteLimit_) { clear(); return; }
    while (!lines_.empty() && (size()>=limit_ || bytes_+bytes>byteLimit_)) {
        bytes_-=lines_.front().size()*sizeof(VTermScreenCell); lines_.pop_front();
    }
    lines_.emplace_back(cells, cells + columns);
    bytes_+=bytes;
}
bool Scrollback::pop(int columns, VTermScreenCell* cells) {
    if (lines_.empty()) return false;
    const auto& l = lines_.back(); std::fill(cells, cells + columns, VTermScreenCell{});
    std::copy_n(l.begin(), std::min(int(l.size()), columns), cells);
    bytes_-=l.size()*sizeof(VTermScreenCell); lines_.pop_back(); return true;
}
}
