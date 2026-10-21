#include "TerminalClientFixture.h"
#include <QByteArray>
#include <QElapsedTimer>
#include <QScopeGuard>
#include <QtGlobal>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <cerrno>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif

int runTerminalClientFixture() {
#ifdef Q_OS_WIN
    const auto input=GetStdHandle(STD_INPUT_HANDLE),output=GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode=0;
    if (!GetConsoleMode(output,&mode) || !SetConsoleMode(output,mode|ENABLE_VIRTUAL_TERMINAL_PROCESSING)) return 2;
    const auto restore=qScopeGuard([&] { SetConsoleMode(output,mode); });
#else
    termios saved{};
    if (tcgetattr(STDIN_FILENO,&saved)) return 2;
    auto raw=saved;
    cfmakeraw(&raw);
    if (tcsetattr(STDIN_FILENO,TCSANOW,&raw)) return 2;
    const auto restore=qScopeGuard([&] { tcsetattr(STDIN_FILENO,TCSANOW,&saved); });
#endif
    std::cout << "\033]10;?\033\\\033]11;?\033\\" << std::flush;
    QByteArray response; QElapsedTimer elapsed; elapsed.start();
    while (elapsed.elapsed()<500) {
#ifdef Q_OS_WIN
        DWORD count=0;
        if (!GetNumberOfConsoleInputEvents(input,&count)) return 3;
        if (!count) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); continue; }
        INPUT_RECORD records[64]; DWORD read=0;
        if (!ReadConsoleInputW(input,records,std::min<DWORD>(count,64),&read)) return 3;
        for (DWORD i=0;i<read;++i) if (records[i].EventType==KEY_EVENT && records[i].Event.KeyEvent.bKeyDown)
            response+=char(records[i].Event.KeyEvent.uChar.UnicodeChar);
#else
        pollfd input{STDIN_FILENO,POLLIN,0};
        const auto ready=poll(&input,1,int(std::max<qint64>(0,500-elapsed.elapsed())));
        if (ready<0) { if (errno==EINTR) continue; return 3; }
        if (!ready) break;
        char bytes[512]; const auto count=read(STDIN_FILENO,bytes,sizeof(bytes));
        if (count<=0) { if (count<0 && errno==EINTR) continue; return 3; }
        response.append(bytes,int(count));
#endif
        if (response.contains("]10;rgb:") && response.contains("]11;rgb:")) break;
    }
    std::cout << "\033[H\033[48;2;40;48;56mPanel\033[K\033[0m\r\n";
    std::cout << "palette-fg=" << response.contains("]10;rgb:") << " palette-bg=" << response.contains("]11;rgb:") << "\r\n";
    std::cout << "\033[6;1Hterm=" << qgetenv("TERM").constData()
              << " color=" << qgetenv("COLORTERM").constData() << std::flush;
    for (int frame=0;frame<6;++frame) {
        std::cout << "\033[3;1H\033[1;38;2;" << 60+frame*30 << ";120;220mL"
                  << "\033[38;2;40;" << 210-frame*20 << ";160moading\033[0m" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
    std::cout << "\033[4;1Hpalette-query-finished\r\n" << std::flush;
    return 0;
}

int runHiddenInputFixture() {
#ifdef Q_OS_WIN
    const auto inputHandle=GetStdHandle(STD_INPUT_HANDLE);
    DWORD saved=0;
    if (!GetConsoleMode(inputHandle,&saved) || !SetConsoleMode(inputHandle,(saved|ENABLE_LINE_INPUT)&~ENABLE_ECHO_INPUT)) return 2;
    const auto restore=qScopeGuard([&] { SetConsoleMode(inputHandle,saved); });
#else
    termios saved{};
    if (tcgetattr(STDIN_FILENO,&saved)) return 2;
    auto hidden=saved;
    hidden.c_lflag=(hidden.c_lflag|ICANON)&~(ECHO|ECHONL);
    if (tcsetattr(STDIN_FILENO,TCSANOW,&hidden)) return 2;
    const auto restore=qScopeGuard([&] { tcsetattr(STDIN_FILENO,TCSANOW,&saved); });
#endif
    std::cout << "Password input probe: " << std::flush;
    std::string input;
    if (!std::getline(std::cin,input)) return 3;
    if (!input.empty() && input.back()=='\r') input.pop_back();
    // This is a dummy value for the no-echo regression, never a real credential.
    const bool matches=input=="terlox-dummy-secret";
    std::fill(input.begin(),input.end(),'\0');
    std::cout << "\r\nsecret-match=" << matches << "\r\n" << std::flush;
    return matches?0:4;
}
