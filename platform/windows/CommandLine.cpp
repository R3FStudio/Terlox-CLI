#include "CommandLine.h"
namespace terlox {
static QString quote(const QString& arg) {
    if (!arg.isEmpty() && !arg.contains(' ') && !arg.contains('\t') && !arg.contains('"')) return arg;
    QString out = "\""; int slashes = 0;
    for (auto c : arg) {
        if (c == '\\') { ++slashes; continue; }
        if (c == '"') { out += QString(slashes * 2 + 1, '\\'); out += c; }
        else { out += QString(slashes, '\\'); out += c; }
        slashes = 0;
    }
    out += QString(slashes * 2, '\\'); return out + '"';
}
QString windowsCommandLine(const QString& exe, const QStringList& args) { QString result = quote(exe); for (const auto& a : args) result += ' ' + quote(a); return result; }
}
