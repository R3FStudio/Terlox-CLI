#pragma once
#include "LaunchOptions.h"
#include <QObject>
#include <memory>
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
    struct Impl; std::unique_ptr<Impl> d;
};
}
