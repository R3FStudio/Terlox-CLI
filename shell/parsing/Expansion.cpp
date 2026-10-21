#include "Expansion.h"
#include <QDir>
namespace terlox::shell {
QString expandVariable(const QString& s, qsizetype& i, int status) {
    if (i + 1 >= s.size()) return "$";
    if (s[i + 1] == '?') { ++i; return QString::number(status); }
    QString name;
    if (s[i + 1] == '{') {
        const auto end = s.indexOf('}', i + 2);
        if (end < 0) throw QString("unclosed variable expansion");
        name = s.mid(i + 2, end - i - 2); i = end;
    } else {
        auto j = i + 1;
        while (j < s.size() && (s[j].isLetterOrNumber() || s[j] == '_')) ++j;
        if (j == i + 1) return "$";
        name = s.mid(i + 1, j - i - 1); i = j - 1;
    }
    return qEnvironmentVariable(name.toUtf8().constData());
}
}
