#pragma once
#include <signal.h>
#include <initializer_list>

namespace terlox {
// Only call at an exec boundary or during standalone shell initialization.
inline bool resetTerminalSignals() {
    struct sigaction action{};
    action.sa_handler=SIG_DFL;
    sigemptyset(&action.sa_mask);
    for (const int signal:{SIGINT,SIGQUIT,SIGTERM,SIGHUP,SIGCHLD,SIGPIPE,SIGTSTP,SIGTTIN,SIGTTOU,SIGWINCH})
        if (sigaction(signal,&action,nullptr)<0) return false;
    sigset_t unblocked;
    sigemptyset(&unblocked);
    return sigprocmask(SIG_SETMASK,&unblocked,nullptr)==0;
}
}
