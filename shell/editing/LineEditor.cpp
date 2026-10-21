#include "LineEditor.h"
#include "Completion.h"
#include "Layout.h"
#include "InputState.h"
#include <iostream>
#include <QDir>
#ifdef Q_OS_WIN
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#endif
namespace terlox::shell {
bool isInteractive() {
#ifdef Q_OS_WIN
    return _isatty(_fileno(stdin));
#else
    return isatty(STDIN_FILENO);
#endif
}
static int readKey() {
#ifdef Q_OS_WIN
    int c = _getwch();
    if (c == 0 || c == 224) { c = _getwch(); switch(c) { case 72: return -1; case 80: return -2; case 75: return -3; case 77: return -4; case 71: return -5; case 79: return -6; case 83: return -7; default: return 0; } }
    if (c >= 0xd800 && c <= 0xdbff) { const int low = _getwch(); if (low >= 0xdc00 && low <= 0xdfff) return 0x10000 + ((c - 0xd800) << 10) + low - 0xdc00; _ungetwch(low); return 0xfffd; }
    return c;
#else
    unsigned char c; if (::read(0, &c, 1) != 1) return 4;
    if (c == 27) {
        // VMIN=0/VTIME=1 prevents a bare Escape from blocking indefinitely.
        unsigned char b; if (::read(0, &b, 1) != 1 || b != '[') return 0;
        if (::read(0, &b, 1) != 1) return 0;
        switch (b) { case 'A': return -1; case 'B': return -2; case 'D': return -3; case 'C': return -4; case 'H': return -5; case 'F': return -6; default: break; }
        if (b>='0' && b<='9') {
            const auto first=b;
            while (::read(0,&b,1)==1) {
                if (b=='~') { if (first=='3') return -7; if (first=='1' || first=='7') return -5; if (first=='4' || first=='8') return -6; return 0; }
                if (b>=0x40 && b<=0x7e) return 0;
            }
        }
        return 0;
    }
    if (c < 128) return c;
    QByteArray bytes(1, char(c)); int extra = c < 224 ? 1 : c < 240 ? 2 : 3;
    while (extra--) { if (::read(0, &c, 1) != 1) break; bytes += char(c); }
    const auto points = QString::fromUtf8(bytes).toUcs4(); return points.isEmpty() ? 0 : int(points[0]);
#endif
}
std::optional<QString> readLine(History& history) {
#ifndef Q_OS_WIN
    struct RawGuard { termios old{}; RawGuard() { tcgetattr(0, &old); auto raw = old; raw.c_lflag &= ~(ICANON | ECHO | ISIG); raw.c_iflag &= ~IXON; raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 1; tcsetattr(0, TCSANOW, &raw); } ~RawGuard() { tcsetattr(0, TCSANOW, &old); } } guard;
#endif
    QString line; qsizetype cursor = 0; history.reset(); int previousCursorRow=0, previousWidth=terminalColumns();
    auto redraw = [&] {
        const QString prompt=QDir::toNativeSeparators(QDir::currentPath())+"> ";
        const int width=terminalColumns(), total=textColumns(prompt+line), target=textColumns(prompt+line.left(cursor));
        QByteArray frame="\033[?2026h\r";
        if (previousCursorRow) frame+="\033["+QByteArray::number(previousCursorRow)+'A';
        frame+="\033[J"+prompt.toUtf8()+line.toUtf8();
        if (total>0 && total%width==0) frame+="\r\n"; else frame+='\r';
        const int endRow=total/width,targetRow=target/width;
        if (endRow>targetRow) frame+="\033["+QByteArray::number(endRow-targetRow)+'A';
        frame+="\033["+QByteArray::number(target%width+1)+'G';
        previousCursorRow=targetRow; previousWidth=width;
        frame+="\033[?2026l";
        std::cout.write(frame.constData(),frame.size()); std::cout.flush();
        publishInputState(line,cursor);
    };
    redraw();
    for (;;) {
#ifndef Q_OS_WIN
        // Wait without busy polling while still allowing timed escape decoding.
        fd_set fds; FD_ZERO(&fds); FD_SET(0, &fds); if (select(1, &fds, nullptr, nullptr, nullptr) <= 0) continue;
#endif
        const int c = readKey();
        if (c == 13 || c == 10) { finishInputState(); std::cout << "\r\n"; return line; }
        if (c == 4 && line.isEmpty()) { finishInputState(); std::cout << "\r\n"; return {}; }
        if (c == 3) { finishInputState(); std::cout << "^C\r\n"; return QString(); }
        if (c == -1) { line = history.previous(line); cursor = line.size(); }
        else if (c == -2) { line = history.next(); cursor = line.size(); }
        else if (c == -3 && cursor > 0) { --cursor; if (cursor > 0 && line[cursor].isLowSurrogate()) --cursor; }
        else if (c == -4 && cursor < line.size()) { if (line[cursor].isHighSurrogate()) ++cursor; ++cursor; }
        else if (c == -5 || c == 1) cursor = 0;
        else if (c == -6 || c == 5) cursor = line.size();
        else if ((c == 8 || c == 127) && cursor > 0) { qsizetype start = cursor - 1; if (start > 0 && line[start].isLowSurrogate()) --start; line.remove(start, cursor - start); cursor = start; }
        else if (c == -7 && cursor < line.size()) line.remove(cursor, line[cursor].isHighSurrogate() ? 2 : 1);
        else if (c == 12) { std::cout << "\033[2J\033[H"; }
        else if (c == 9) {
            const auto start = line.lastIndexOf(' ', cursor - 1) + 1;
            const auto prefix = line.mid(start, cursor - start); const auto candidates = completions(prefix);
            if (candidates.size() == 1) { line.replace(start, cursor - start, candidates[0]); cursor = start + candidates[0].size(); }
            else if (!candidates.isEmpty()) std::cout << "\r\n" << candidates.join("  ").toUtf8().constData() << "\r\n";
        }
        else if (c >= 32) {
            const char32_t cp = c; const auto text = QString::fromUcs4(&cp, 1);
            const bool append=cursor==line.size() && terminalColumns()==previousWidth;
            const auto oldWidth=textColumns(QDir::toNativeSeparators(QDir::currentPath())+"> "+line);
            line.insert(cursor, text); cursor += text.size();
            if (append) {
                // Do not erase/repaint existing text on ordinary typing.
                const auto bytes=text.toUtf8(); std::cout.write(bytes.constData(),bytes.size()); std::cout.flush();
                const auto cells=oldWidth+textColumns(text); previousCursorRow=cells/previousWidth;
                if (cells>0 && cells%previousWidth==0) std::cout << "\r\n";
                publishInputState(line,cursor);
                continue;
            }
        }
        redraw();
    }
}
}
