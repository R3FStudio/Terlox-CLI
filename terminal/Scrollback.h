#pragma once
#include <vterm.h>
#include <QTemporaryFile>
#include <QString>
#include <deque>
#include <vector>
#include <functional>
namespace terlox {
using CellLine = std::vector<VTermScreenCell>;
class Scrollback {
public:
    // The line limit and byte budget bound the RAM cache, not retained history.
    // Zero lines explicitly disables scrollback. Archived rows live until clear
    // or session destruction in private, automatically removed temporary files.
    explicit Scrollback(int limit = 10000, size_t byteLimit = 32*1024*1024);
    bool push(int columns, const VTermScreenCell* cells);
    bool pop(int columns, VTermScreenCell* cells);
    void clear();
    size_t bytes() const { return bytes_+loaded_.capacity()*sizeof(VTermScreenCell); }
    qint64 diskBytes() const;
    int size() const { return archived_+int(lines_.size()); }
    const CellLine& line(int index) const;
    const QString& error() const { return error_; }
    void setErrorHandler(std::function<void(const QString&)> handler) { onError_=std::move(handler); }
private:
    bool archive(const CellLine& line);
    bool load(int index) const;
    bool fail(const QString& message) const;
    std::deque<CellLine> lines_;
    int limit_, archived_=0;
    size_t residentLimit_, bytes_=0;
    mutable QTemporaryFile data_, index_;
    mutable CellLine loaded_;
    mutable int loadedIndex_=-1;
    mutable QString error_;
    std::function<void(const QString&)> onError_;
};
}
