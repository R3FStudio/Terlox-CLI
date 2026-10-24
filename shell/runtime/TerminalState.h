#pragma once
#include <QtGlobal>
#ifndef Q_OS_WIN
#include <termios.h>
#endif

namespace terlox::shell {
class TerminalState {
public:
    TerminalState();
    ~TerminalState();
    TerminalState(const TerminalState&)=delete;
    TerminalState& operator=(const TerminalState&)=delete;
private:
#ifndef Q_OS_WIN
    termios modes_{};
    bool saved_=false;
#endif
};
}
