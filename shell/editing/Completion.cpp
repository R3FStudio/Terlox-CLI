#include "Completion.h"
#include <QDir>
#include <QFileInfo>
namespace terlox::shell {
QStringList completions(const QString& prefix) {
    if (QString("/info-terlox").startsWith(prefix) && prefix.startsWith('/')) return {"/info-terlox"};
    const QFileInfo file(prefix); QDir dir(file.path()); QStringList out;
    const QString lead = prefix.left(prefix.size() - file.fileName().size());
    for (const auto& e : dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot))
        if (e.fileName().startsWith(file.fileName(), Qt::CaseInsensitive)) out << lead + e.fileName() + (e.isDir() ? "/" : "");
    if (!prefix.contains('/') && !prefix.contains('\\')) {
        for (const auto& path : qEnvironmentVariable("PATH").split(QDir::listSeparator()))
            for (const auto& e : QDir(path).entryInfoList(QDir::Files | QDir::Executable))
                if (e.fileName().startsWith(prefix, Qt::CaseInsensitive)) out << e.fileName();
        for (const auto& b : {"cd", "pwd", "echo", "clear", "exit", "export", "unset", "history"
#ifdef Q_OS_WIN
            ,"ls","dir","cat","copy","move","del","mkdir","rmdir","ps","cmd","chdir","Set-Location"
#endif
        })
            if (QString(b).startsWith(prefix)) out << b;
    }
    out.removeDuplicates(); out.sort(); return out;
}
}
