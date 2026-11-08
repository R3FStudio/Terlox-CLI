#include "platform/Process.h"
#include "platform/Environment.h"
#include "CommandLine.h"
#include "platform/InboundSession.h"
#include "integration/windows/ConptyApi.h"
#include <utility>
#include <QMetaObject>
#include <QProcessEnvironment>
#include <windows.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <atomic>
namespace terlox {
struct Handle {
    HANDLE value = nullptr;
    ~Handle() { reset(); }
    void reset(HANDLE h = nullptr) { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); value = h; }
};
struct Process::Impl {
    HPCON console = nullptr;
    Handle input, output, process, job, client;
    bool inbound=false, packagedConsole=false;
    std::thread reader, writer, waiter;
    std::mutex queueMutex, consoleMutex;
    std::condition_variable ready; std::deque<QByteArray> queue;
    std::optional<COORD> pendingSize;
    std::atomic<bool> stopping{false};
};
Process::Process(QObject* parent) : QObject(parent), d(std::make_unique<Impl>()) {}
Process::~Process() { stop(); }
bool Process::start(const LaunchOptions& options, QString& error) {
    if (d->process.value) { error = "Session is already started"; return false; }
    auto fail = [&](const QString& operation) { error = operation + QString(" (Windows error %1)").arg(GetLastError()); stop(); return false; };
    if (options.inbound) {
        auto& incoming=*options.inbound; const auto& api=ConptyApi::instance();
        if (!api.available()) { error="The default-terminal ConPTY runtime is unavailable."; return false; }
        if (!DuplicateHandle(GetCurrentProcess(),incoming.server,GetCurrentProcess(),&d->process.value,0,FALSE,DUPLICATE_SAME_ACCESS)) return fail("Duplicate console host");
        const auto hr=api.pack(incoming.server,incoming.reference,incoming.signal,&d->console);
        if (FAILED(hr)) { error=QString("Could not attach the incoming console (0x%1).").arg(quint32(hr),0,16); stop(); return false; }
        incoming.server=incoming.reference=incoming.signal=nullptr; d->inbound=true; d->packagedConsole=true;
        d->input.value=std::exchange(incoming.input,nullptr); d->output.value=std::exchange(incoming.output,nullptr); d->client.value=std::exchange(incoming.client,nullptr);
        d->stopping=false; api.resize(d->console,{SHORT(options.columns),SHORT(options.rows)});
        startIo(); api.release(d->console); return true;
    }
    Handle inputRead, outputWrite;
    if (!CreatePipe(&inputRead.value, &d->input.value, nullptr, 0) || !CreatePipe(&d->output.value, &outputWrite.value, nullptr, 0)) return fail("CreatePipe");
    const auto& api=ConptyApi::instance();
    d->packagedConsole=api.canCreate();
    // Use the shipped console host for the same VT transport on all supported
    // Windows releases. The system API remains a development fallback.
    auto hr = d->packagedConsole?api.create({SHORT(options.columns),SHORT(options.rows)},inputRead.value,outputWrite.value,0,&d->console)
        :CreatePseudoConsole({SHORT(options.columns), SHORT(options.rows)}, inputRead.value, outputWrite.value, 0, &d->console);
    if (FAILED(hr)) { error = QString("ConPTY failed (HRESULT 0x%1); Windows 10 1809 or newer is required").arg(quint32(hr), 0, 16); stop(); return false; }
    STARTUPINFOEXW si{}; si.StartupInfo.cb = sizeof(si);
    // Explicit null handles prevent redirected parent streams from bypassing ConPTY.
    si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    SIZE_T size = 0; InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
    std::vector<unsigned char> storage(size); si.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
    if (!InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &size)) return fail("InitializeProcThreadAttributeList");
    const bool updated = UpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, d->console, sizeof(d->console), nullptr, nullptr);
    auto command = (options.nativeArguments.isEmpty()?windowsCommandLine(options.executable,options.arguments):windowsCommandLine(options.executable,{})+' '+options.nativeArguments).toStdWString();
    auto environment = interactiveTerminalEnvironment();
    environment.insert("TERM_PROGRAM","Terlox"); environment.insert("TERM_PROGRAM_VERSION",TERLOX_BUILD_VERSION); environment.insert("TERLOX_APP_CONTROL","777"); environment.insert("TERLOX_GRAPHICS","iterm2");
    environment.insert("TERM", "xterm-256color"); environment.insert("COLORTERM", "truecolor");
    auto variables = environment.toStringList(); variables.sort(Qt::CaseInsensitive);
    std::wstring environmentBlock;
    for (const auto& variable : variables) { environmentBlock += variable.toStdWString(); environmentBlock += L'\0'; }
    environmentBlock += L'\0';
    auto directory = options.directory.toStdWString(); PROCESS_INFORMATION pi{};
    d->job.value = CreateJobObjectW(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{}; limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!d->job.value || !SetInformationJobObject(d->job.value, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) { DeleteProcThreadAttributeList(si.lpAttributeList); return fail("Create session job"); }
    const bool created = updated && CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE,
        EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED, environmentBlock.data(),
        directory.empty() ? nullptr : directory.c_str(), &si.StartupInfo, &pi);
    const auto creationError = GetLastError(); DeleteProcThreadAttributeList(si.lpAttributeList);
    if (!created) { SetLastError(creationError); return fail("CreateProcess"); }
    d->process.value = pi.hProcess; Handle primary; primary.value = pi.hThread;
    if (!AssignProcessToJobObject(d->job.value, pi.hProcess)) { TerminateProcess(pi.hProcess, 1); return fail("AssignProcessToJobObject"); }
    inputRead.reset(); outputWrite.reset(); d->stopping = false;
    startIo();
    ResumeThread(primary.value); return true;
}
void Process::startIo() {
    d->reader = std::thread([this] {
        char buffer[32768]; DWORD count;
        while (ReadFile(d->output.value, buffer, sizeof(buffer), &count, nullptr) && count) {
            const QByteArray bytes(buffer, count);
            // During shutdown keep draining the pipe: ClosePseudoConsole can
            // wait for its final output even after the client was terminated.
            queueOutput(bytes);
        }
    });
    d->writer = std::thread([this] {
        for (;;) {
            QByteArray bytes; std::optional<COORD> size;
            { std::unique_lock lock(d->queueMutex); d->ready.wait(lock, [this] { return d->stopping || !d->queue.empty() || d->pendingSize.has_value(); });
              if (d->stopping) return;
              size=d->pendingSize; d->pendingSize.reset();
              if (!d->queue.empty()) { bytes=std::move(d->queue.front()); d->queue.pop_front(); } }
            if (size) {
                std::lock_guard lock(d->consoleMutex);
                if (d->console) {
                    if (d->packagedConsole) ConptyApi::instance().resize(d->console,*size);
                    else ResizePseudoConsole(d->console,*size);
                }
            }
            qsizetype offset = 0;
            while (offset < bytes.size() && !d->stopping) { DWORD written = 0; if (!WriteFile(d->input.value, bytes.constData() + offset, DWORD(bytes.size() - offset), &written, nullptr) || !written) return; offset += written; }
        }
    });
    d->waiter = std::thread([this] {
        WaitForSingleObject(d->process.value, INFINITE); DWORD code = 0; GetExitCodeProcess(d->process.value, &code);
        if (d->inbound && d->client.value) GetExitCodeProcess(d->client.value,&code);
        if (d->job.value) TerminateJobObject(d->job.value, code);
        HPCON console;
        { std::lock_guard lock(d->consoleMutex); console = d->console; d->console = nullptr; }
        if (console) { if (d->packagedConsole) ConptyApi::instance().close(console); else ClosePseudoConsole(console); } // Reader keeps draining during teardown.
        if (d->reader.joinable()) d->reader.join();
        finishOutput(int(code));
    });
}
void Process::write(const QByteArray& bytes) {
    { std::lock_guard lock(d->queueMutex); if (d->stopping) return; d->queue.push_back(bytes); } d->ready.notify_one();
}
void Process::resize(int rows, int columns) {
    // ConPTY resizing can wait for the console host. Never block the UI thread
    // (which also drains output) while that native operation is in progress.
    { std::lock_guard lock(d->queueMutex); d->pendingSize=COORD{SHORT(columns),SHORT(rows)}; }
    d->ready.notify_one();
}
void Process::stop() {
    stopOutput();
    d->stopping = true; d->ready.notify_all();
    if (d->job.value) TerminateJobObject(d->job.value, 0);
    if (d->writer.joinable()) { CancelSynchronousIo(d->writer.native_handle()); d->writer.join(); }
    if (d->inbound) {
        HPCON console;
        { std::lock_guard lock(d->consoleMutex); console=d->console; d->console=nullptr; }
        if (console) ConptyApi::instance().close(console);
    }
    if (d->waiter.joinable()) d->waiter.join();
    if (d->console) { if (d->packagedConsole) ConptyApi::instance().close(d->console); else ClosePseudoConsole(d->console); d->console = nullptr; }
    if (d->reader.joinable()) d->reader.join();
    d->input.reset(); d->output.reset(); d->process.reset(); d->job.reset(); d->client.reset(); d->inbound=false; d->packagedConsole=false;
}
}
