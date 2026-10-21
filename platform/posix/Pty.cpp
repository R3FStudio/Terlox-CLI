#include "platform/Process.h"
#include "platform/Environment.h"
#include <QFile>
#include <QMetaObject>
#include <QStandardPaths>
#include <atomic>
#include <thread>
#include <mutex>
#include <deque>
#include <vector>
#include <poll.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <cstring>
#ifdef Q_OS_MACOS
#include <util.h>
#else
#include <pty.h>
#endif
namespace terlox {
struct Process::Impl {
    int master = -1, wake[2]{-1,-1}; pid_t child = -1;
    std::thread io, waiter; std::atomic<bool> stopping{false}, ended{false};
    std::mutex mutex; std::deque<QByteArray> pending;
};
Process::Process(QObject* p) : QObject(p), d(std::make_unique<Impl>()) {}
Process::~Process() { stop(); }
bool Process::start(const LaunchOptions& o, QString& error) {
    auto exe = o.executable.contains('/') ? o.executable : QStandardPaths::findExecutable(o.executable);
    if (exe.isEmpty()) { error = "Executable not found: " + o.executable; return false; }
    QList<QByteArray> storage{QFile::encodeName(exe)}; for (const auto& a : o.arguments) storage << a.toUtf8();
    std::vector<char*> argv; for (auto& a : storage) argv.push_back(a.data()); argv.push_back(nullptr);
    auto environment=interactiveTerminalEnvironment();
    environment.insert("TERM_PROGRAM","Terlox"); environment.insert("TERM_PROGRAM_VERSION",TERLOX_BUILD_VERSION);
    environment.insert("TERLOX_APP_CONTROL","777"); environment.insert("TERLOX_GRAPHICS","kitty,iterm2");
    QList<QByteArray> variables; for (const auto& value:environment.toStringList()) variables << value.toUtf8();
    std::vector<char*> envp; for (auto& variable:variables) envp.push_back(variable.data()); envp.push_back(nullptr);
    const auto cwd = QFile::encodeName(o.directory);
    if (::pipe(d->wake)) { error = QString::fromLocal8Bit(strerror(errno)); return false; }
    for (int fd : d->wake) { fcntl(fd, F_SETFD, FD_CLOEXEC); fcntl(fd, F_SETFL, O_NONBLOCK); }
    winsize size{}; size.ws_row = o.rows; size.ws_col = o.columns;
    d->child = forkpty(&d->master, nullptr, nullptr, &size);
    if (d->child == 0) {
        close(d->wake[0]); close(d->wake[1]);
        signal(SIGINT, SIG_DFL); signal(SIGTERM, SIG_DFL); signal(SIGHUP, SIG_DFL);
        if (!cwd.isEmpty() && chdir(cwd.constData())) { const char msg[] = "terlox: cannot enter working directory\r\n"; ::write(2, msg, sizeof(msg)-1); _exit(126); }
        execve(argv[0], argv.data(),envp.data()); const char msg[] = "terlox: exec failed\r\n"; ::write(2, msg, sizeof(msg)-1); _exit(127);
    }
    if (d->child < 0) { error = QString::fromLocal8Bit(strerror(errno)); stop(); return false; }
    fcntl(d->master, F_SETFD, FD_CLOEXEC); fcntl(d->master, F_SETFL, O_NONBLOCK);
    d->io = std::thread([this] {
        QByteArray outgoing;
        while (!d->stopping) {
            { std::lock_guard lock(d->mutex); while (!d->pending.empty()) { outgoing += d->pending.front(); d->pending.pop_front(); } }
            pollfd fds[2]{{d->master, short(POLLIN | (outgoing.isEmpty() ? 0 : POLLOUT)), 0}, {d->wake[0], POLLIN, 0}};
            if (poll(fds, 2, -1) < 0) { if (errno == EINTR) continue; break; }
            if (fds[1].revents & POLLIN) { char b[64]; while (::read(d->wake[0], b, sizeof(b)) > 0) {} }
            if (fds[0].revents & POLLOUT) { const auto n = ::write(d->master, outgoing.constData(), outgoing.size()); if (n > 0) outgoing.remove(0, n); }
            if (fds[0].revents & (POLLIN | POLLHUP)) {
                char b[32768]; ssize_t n;
                while ((n = ::read(d->master, b, sizeof(b))) > 0) { const QByteArray bytes(b, n); QMetaObject::invokeMethod(this, [this, bytes] { emit received(bytes); }); }
                if (n == 0 || (n < 0 && errno == EIO)) break;
            }
            if (d->ended && !(fds[0].revents & POLLIN)) break;
        }
    });
    d->waiter = std::thread([this] {
        int status = 0; while (waitpid(d->child, &status, 0) < 0 && errno == EINTR) {}
        kill(-d->child, SIGHUP); d->ended = true; const char wake = 0; ::write(d->wake[1], &wake, 1);
        const int code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
        QMetaObject::invokeMethod(this, [this, code] { emit exited(code); });
    });
    return true;
}
void Process::write(const QByteArray& bytes) { if (d->stopping || d->ended) return; { std::lock_guard lock(d->mutex); d->pending.push_back(bytes); } const char wake = 0; ::write(d->wake[1], &wake, 1); }
void Process::resize(int rows, int columns) { if (d->master >= 0) { winsize s{}; s.ws_row = rows; s.ws_col = columns; ioctl(d->master, TIOCSWINSZ, &s); } }
void Process::stop() {
    d->stopping = true;
    if (d->child > 0 && !d->ended) kill(-d->child, SIGKILL);
    if (d->wake[1] >= 0) { const char wake = 0; ::write(d->wake[1], &wake, 1); }
    if (d->waiter.joinable()) d->waiter.join();
    if (d->io.joinable()) d->io.join();
    for (int* fd : {&d->master, &d->wake[0], &d->wake[1]}) if (*fd >= 0) { close(*fd); *fd = -1; }
}
}
