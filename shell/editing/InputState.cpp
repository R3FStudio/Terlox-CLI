#include "InputState.h"
#include "Layout.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>
#ifdef Q_OS_WIN
#include <conio.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif
namespace terlox::shell {
static void publish(const QJsonObject& state) {
    const auto message="\033]777;terlox-input;"+QJsonDocument(state).toJson(QJsonDocument::Compact).toBase64()+"\033\\";
    std::cout.write(message.constData(),message.size()); std::cout.flush();
}
void publishInputState(const QString& line,qsizetype cursor) {
    if (line.size()>8192) { finishInputState(); return; }
    // Publish after queued keystrokes drain, including a paste, to avoid
    // repeating the complete buffer for every character in a batch.
#ifdef Q_OS_WIN
    if (_kbhit()) return;
#else
    int pending=0; if (ioctl(STDIN_FILENO,FIONREAD,&pending)==0 && pending>0) return;
#endif
    QJsonArray widths;
    for (const char32_t point:line.toUcs4()) widths.append(textColumns(QString::fromUcs4(&point,1)));
    publish({{"active",true},{"text",line},{"widths",widths},{"cursor",int(line.left(cursor).toUcs4().size())}});
}
void finishInputState() { publish({{"active",false}}); }
}
