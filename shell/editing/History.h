#pragma once
#include <QStringList>
namespace terlox::shell {
class History {
public:
    void add(const QString& line);
    QString previous(const QString& draft);
    QString next();
    void reset();
    void load(const QString& path);
    void save(const QString& path) const;
    const QStringList& entries() const { return lines_; }
private:
    QStringList lines_; QString draft_; qsizetype index_ = 0;
};
}
