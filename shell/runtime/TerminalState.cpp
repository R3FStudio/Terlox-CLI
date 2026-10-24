#include "TerminalState.h"
#ifndef Q_OS_WIN
#include <cerrno>
#include <signal.h>
#include <unistd.h>
#endif

namespace terlox::shell {
TerminalState::TerminalState() {
#ifndef Q_OS_WIN
    saved_=tcgetattr(STDIN_FILENO,&modes_)==0;
#endif
}
TerminalState::~TerminalState() {
#ifndef Q_OS_WIN
    if (!saved_) return;
    sigset_t blocked, previous;
    sigemptyset(&blocked); sigaddset(&blocked,SIGTTOU);
    if (sigprocmask(SIG_BLOCK,&blocked,&previous)<0) return;
    if (tcgetpgrp(STDIN_FILENO)>0) {
        while (tcsetpgrp(STDIN_FILENO,getpgrp())<0 && errno==EINTR) {}
        while (tcsetattr(STDIN_FILENO,TCSANOW,&modes_)<0 && errno==EINTR) {}
    }
    sigprocmask(SIG_SETMASK,&previous,nullptr);
#endif
}
}
