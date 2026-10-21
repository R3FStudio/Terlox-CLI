#pragma once
#include <vterm.h>
#include <deque>
#include <vector>
namespace terlox {
using CellLine = std::vector<VTermScreenCell>;
class Scrollback {
public:
    explicit Scrollback(int limit = 10000) : limit_(limit) {}
    void push(int columns, const VTermScreenCell* cells);
    bool pop(int columns, VTermScreenCell* cells);
    void clear() { lines_.clear(); }
    int size() const { return int(lines_.size()); }
    const CellLine& line(int index) const { return lines_.at(index); }
private:
    std::deque<CellLine> lines_; int limit_;
};
}
