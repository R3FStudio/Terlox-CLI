#include "History.h"
#include <QFile>
#include <QSaveFile>
namespace terlox::shell {
void History::add(const QString& s) { if (!s.trimmed().isEmpty() && (lines_.isEmpty() || lines_.last() != s)) lines_ << s; while (lines_.size() > 2000) lines_.removeFirst(); reset(); }
void History::reset() { index_ = lines_.size(); draft_.clear(); }
QString History::previous(const QString& draft) { if (index_ == lines_.size()) draft_ = draft; if (index_ > 0) --index_; return index_ < lines_.size() ? lines_[index_] : draft_; }
QString History::next() { if (index_ < lines_.size()) ++index_; return index_ < lines_.size() ? lines_[index_] : draft_; }
void History::load(const QString& path) { QFile f(path); if (f.open(QIODevice::ReadOnly)) for (const auto& l : QString::fromUtf8(f.readAll()).split('\n')) add(l); reset(); }
void History::save(const QString& path) const { QSaveFile f(path); if (f.open(QIODevice::WriteOnly)) { f.write(lines_.join('\n').toUtf8()); f.commit(); } }
}
