#include "UnixProcessFixture.h"
#include <QCoreApplication>
#include <QFile>
#include <iostream>
#ifndef Q_OS_WIN
#include <cerrno>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#endif

int runUnixProcessFixture() {
#ifndef Q_OS_WIN
    const auto arguments=QCoreApplication::arguments();
    if (arguments.contains("--unix-inherited-signals-child")) {
        signal(SIGCHLD,SIG_IGN);
        sigset_t blocked; sigemptyset(&blocked);
        sigaddset(&blocked,SIGCHLD); sigaddset(&blocked,SIGINT);
        sigprocmask(SIG_BLOCK,&blocked,nullptr);
        const auto shell=QFile::encodeName(QCoreApplication::applicationDirPath()+"/terloxsh");
        const auto command='"'+QCoreApplication::applicationFilePath()+"\" --unix-state-child";
        const auto bytes=command.toUtf8();
        execl(shell.constData(),shell.constData(),"-c",bytes.constData(),nullptr);
        return 127;
    }
    if (arguments.contains("--unix-damaged-terminal-child")) {
        termios modes{};
        if (tcgetattr(STDIN_FILENO,&modes)<0) return 2;
        modes.c_lflag&=~(ICANON|ECHO|ISIG);
        if (tcsetattr(STDIN_FILENO,TCSANOW,&modes)<0) return 3;
        sigset_t blocked; sigemptyset(&blocked); sigaddset(&blocked,SIGTTOU);
        sigprocmask(SIG_BLOCK,&blocked,nullptr);
        if (setpgid(0,0)<0 || tcsetpgrp(STDIN_FILENO,getpgrp())<0) return 4;
        std::cout << "terminal-modes-changed\n" << std::flush;
        return 0;
    }
    if (arguments.contains("--unix-state-child")) {
        struct sigaction action{}; sigaction(SIGCHLD,nullptr,&action);
        sigset_t blocked; sigprocmask(SIG_SETMASK,nullptr,&blocked);
        const bool clean=action.sa_handler==SIG_DFL && !sigismember(&blocked,SIGCHLD)
            && !sigismember(&blocked,SIGINT) && !sigismember(&blocked,SIGTERM);
        std::cout << "signals-clean=" << clean << '\n';
        const auto child=fork();
        if (child==0) _exit(37);
        int status=0; pid_t waited=-1;
        if (child>0) do { waited=waitpid(child,&status,0); } while (waited<0 && errno==EINTR);
        const bool reaped=waited==child && waited>0 && WIFEXITED(status) && WEXITSTATUS(status)==37;
        std::cout << "child-wait-ok=" << reaped << '\n';
        if (isatty(STDIN_FILENO)) {
            termios modes{};
            const bool terminal=tcgetattr(STDIN_FILENO,&modes)==0
                && (modes.c_lflag&(ICANON|ECHO|ISIG))==(ICANON|ECHO|ISIG)
                && tcgetpgrp(STDIN_FILENO)==getpgrp();
            std::cout << "terminal-ready=" << terminal << '\n';
            if (!terminal) return 3;
        }
        return clean && reaped?0:2;
    }
#endif
    return -1;
}
