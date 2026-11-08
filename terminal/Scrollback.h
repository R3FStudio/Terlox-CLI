#pragma once
#include <vterm.h>
#include <deque>
#include <vector>
namespace terlox {
using CellLine = std::vector<VTermScreenCell>;
class Scrollback {
public:
    explicit Scrollback(int limit = 10000, size_t byteLimit = 32*1024*1024) : limit_(limit), byteLimit_(byteLimit) {}
    void push(int columns, const VTermScreenCell* cells);
    bool pop(int columns, VTermScreenCell* cells);
    void clear() { lines_.clear(); bytes_=0; }
    size_t bytes() const { return bytes_; }
    int size() const { return int(lines_.size()); }
    const CellLine& line(int index) const { return lines_.at(index); }
private:
    std::deque<CellLine> lines_; int limit_;
    size_t byteLimit_, bytes_=0;
};
}
