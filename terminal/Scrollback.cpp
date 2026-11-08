#include "Scrollback.h"
#include <QDataStream>
#include <QDir>
#include <QtEndian>
#include <QDebug>
#include <algorithm>
#include <cstring>
#include <limits>
#include <type_traits>
namespace terlox {
static_assert(std::is_trivially_copyable_v<VTermScreenCell>);
Scrollback::Scrollback(int limit,size_t byteLimit)
    : limit_(limit), residentLimit_(byteLimit-std::min(byteLimit/4,size_t(2048)*sizeof(VTermScreenCell))),
      data_(QDir::tempPath()+"/terlox-scrollback-data-XXXXXX"),
      index_(QDir::tempPath()+"/terlox-scrollback-index-XXXXXX") {}
bool Scrollback::fail(const QString& message) const {
    if (error_.isEmpty()) {
        error_="Scrollback storage error: "+message+". Existing history is retained, but additional history may be unavailable. Check free temporary-disk space; clear the terminal to retry.";
        qWarning().noquote() << error_;
        if (onError_) onError_(error_);
    }
    return false;
}
bool Scrollback::archive(const CellLine& line) {
    if ((!data_.isOpen() && !data_.open()) || (!index_.isOpen() && !index_.open()))
        return fail("cannot create private temporary files");
    const auto offset=data_.size();
    const auto raw=QByteArray::fromRawData(reinterpret_cast<const char*>(line.data()),qsizetype(line.size()*sizeof(VTermScreenCell)));
    const auto compressed=qCompress(raw,1);
    QByteArray record; QDataStream stream(&record,QIODevice::WriteOnly);
    stream << qint32(line.size()) << quint32(compressed.size());
    record+=compressed;
    QByteArray entry; QDataStream offsets(&entry,QIODevice::WriteOnly); offsets << offset;
    if (!data_.seek(offset) || data_.write(record)!=record.size() ||
        !index_.seek(qint64(archived_)*8) || index_.write(entry)!=entry.size()) {
        data_.resize(offset); index_.resize(qint64(archived_)*8);
        return fail("cannot write history");
    }
    ++archived_; return true;
}
bool Scrollback::push(int columns,const VTermScreenCell* cells) {
    if (limit_<=0 || columns<=0) return true;
    if (!error_.isEmpty()) return false;
    if (size()==std::numeric_limits<int>::max()) return fail("maximum addressable line count reached");
    lines_.emplace_back(cells,cells+columns); bytes_+=size_t(columns)*sizeof(VTermScreenCell);
    while (!lines_.empty() && (int(lines_.size())>limit_ || bytes_>residentLimit_)) {
        if (!archive(lines_.front())) return false;
        bytes_-=lines_.front().size()*sizeof(VTermScreenCell); lines_.pop_front();
    }
    return true;
}
bool Scrollback::load(int index) const {
    if (loadedIndex_==index) return true;
    if (!index_.seek(qint64(index)*8)) return fail("cannot locate archived line");
    QDataStream offsets(&index_); qint64 offset=-1; offsets >> offset;
    if (offsets.status()!=QDataStream::Ok || offset<0 || offset>=data_.size() || !data_.seek(offset))
        return fail("invalid history index");
    QDataStream stream(&data_); qint32 columns=0; quint32 length=0; stream >> columns >> length;
    if (stream.status()!=QDataStream::Ok || columns<=0 || columns>1024*1024 || length<4 || length>64*1024*1024 || length>data_.size()-data_.pos())
        return fail("invalid archived line");
    const auto encoded=data_.read(length);
    const auto expected=size_t(columns)*sizeof(VTermScreenCell);
    if (encoded.size()!=length || qFromBigEndian<quint32>(encoded.constData())!=expected)
        return fail("incomplete archived line");
    const auto raw=qUncompress(encoded);
    if (size_t(raw.size())!=expected) return fail("cannot decompress archived line");
    if (loaded_.size()!=size_t(columns)) CellLine(size_t(columns)).swap(loaded_);
    std::memcpy(loaded_.data(),raw.constData(),expected);
    loadedIndex_=index; return true;
}
const CellLine& Scrollback::line(int index) const {
    static const CellLine empty;
    if (index<0 || index>=size()) return empty;
    if (index>=archived_) return lines_[size_t(index-archived_)];
    return load(index)?loaded_:empty;
}
bool Scrollback::pop(int columns,VTermScreenCell* cells) {
    if (!size()) return false;
    const auto& last=line(size()-1);
    if (last.empty()) return false;
    std::fill(cells,cells+columns,VTermScreenCell{});
    std::copy_n(last.begin(),std::min(int(last.size()),columns),cells);
    if (!lines_.empty()) { bytes_-=lines_.back().size()*sizeof(VTermScreenCell); lines_.pop_back(); }
    else {
        if (!index_.seek(qint64(archived_-1)*8)) return fail("cannot pop archived line");
        QDataStream stream(&index_); qint64 offset=-1; stream >> offset;
        if (stream.status()!=QDataStream::Ok || offset<0 || !data_.resize(offset) || !index_.resize(qint64(archived_-1)*8))
            return fail("cannot truncate popped history");
        --archived_; loadedIndex_=-1;
    }
    return true;
}
void Scrollback::clear() {
    lines_.clear(); CellLine().swap(loaded_); bytes_=0; archived_=0; loadedIndex_=-1;
    data_.close(); if (!data_.fileName().isEmpty()) data_.remove();
    index_.close(); if (!index_.fileName().isEmpty()) index_.remove(); error_.clear();
}
qint64 Scrollback::diskBytes() const { return (data_.isOpen()?data_.size():0)+(index_.isOpen()?index_.size():0); }
}
