#include "Process.h"
#include <QMetaObject>
#include <QTimer>
namespace terlox {
bool Process::queueOutput(const QByteArray& bytes) {
    std::unique_lock lock(outputMutex_);
    // Backpressure bounds queued bytes without dropping terminal escape
    // sequences or output. The OS pipe pauses the producer while we catch up.
    outputReady_.wait(lock,[&] { return outputStopping_ || outputPending_.size()+bytes.size()<=1024*1024; });
    if (outputStopping_) return false;
    outputPending_+=bytes;
    if (!outputScheduled_) {
        outputScheduled_=true;
        QMetaObject::invokeMethod(this,[this] { drainOutput(); },Qt::QueuedConnection);
    }
    return true;
}
void Process::drainOutput() {
    QByteArray bytes; bool more;
    {
        std::lock_guard lock(outputMutex_);
        if (outputStopping_) return;
        bytes=outputPending_.left(64*1024); outputPending_.remove(0,bytes.size());
        more=!outputPending_.isEmpty(); outputScheduled_=more;
    }
    outputReady_.notify_all();
    if (!bytes.isEmpty()) emit received(bytes);
    if (more) QTimer::singleShot(0,this,[this] { drainOutput(); });
    else if (outputExit_) { const auto code=*outputExit_; outputExit_.reset(); emit exited(code); }
}
void Process::finishOutput(int status) {
    QMetaObject::invokeMethod(this,[this,status] {
        bool pending;
        { std::lock_guard lock(outputMutex_); if (outputStopping_) return; pending=outputScheduled_; }
        if (pending) outputExit_=status;
        else emit exited(status);
    },Qt::QueuedConnection);
}
void Process::stopOutput() {
    { std::lock_guard lock(outputMutex_); outputStopping_=true; outputPending_.clear(); }
    outputReady_.notify_all();
}
}
