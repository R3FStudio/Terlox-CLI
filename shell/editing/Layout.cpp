#include "Layout.h"
#include <QList>
#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif
namespace terlox::shell {
int terminalColumns() {
#ifdef Q_OS_WIN
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&info)) return std::max(1,int(info.dwSize.X));
#else
    winsize size{}; if (ioctl(STDOUT_FILENO,TIOCGWINSZ,&size)==0 && size.ws_col) return size.ws_col;
#endif
    return 80;
}
int textColumns(const QString& text) {
    int cells=0;
    for (const auto c:text.toUcs4()) {
        const auto category=QChar::category(c);
        if (category==QChar::Mark_NonSpacing || category==QChar::Mark_Enclosing || c==0x200d || c==0xfe0f) continue;
        const bool wide=(c>=0x1100 && c<=0x115f) || (c>=0x2e80 && c<=0xa4cf) || (c>=0xac00 && c<=0xd7a3) || (c>=0xf900 && c<=0xfaff) || (c>=0xfe10 && c<=0xfe6f) || (c>=0xff00 && c<=0xff60) || (c>=0x1f300 && c<=0x1faff) || (c>=0x20000 && c<=0x3ffff);
        cells+=wide?2:1;
    }
    return cells;
}
}
