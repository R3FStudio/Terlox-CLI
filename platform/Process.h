#pragma once
#include "LaunchOptions.h"
#include <QObject>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <optional>
namespace terlox {
// One native session; byte delivery is queued onto its owning Qt thread.
class Process : public QObject {
    Q_OBJECT
public:
    explicit Process(QObject* parent = nullptr);
    ~Process() override;
    bool start(const LaunchOptions& options, QString& error);
    void write(const QByteArray& bytes);
    void resize(int rows, int columns);
    void stop();
signals:
    void received(const QByteArray& bytes);
    void exited(int status);
private:
    void startIo();
    bool queueOutput(const QByteArray& bytes);
    void finishOutput(int status);
    void drainOutput();
    void stopOutput();
    std::mutex outputMutex_;
    std::condition_variable outputReady_;
    QByteArray outputPending_;
    bool outputScheduled_=false, outputStopping_=false;
    std::optional<int> outputExit_;
    struct Impl; std::unique_ptr<Impl> d;
};
}
