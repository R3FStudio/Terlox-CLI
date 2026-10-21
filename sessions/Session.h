#pragma once
#include "platform/Process.h"
#include "terminal/Screen.h"
namespace terlox {
class Session : public QObject {
    Q_OBJECT
public:
    Session(LaunchOptions launch, int scrollback, QObject* parent = nullptr);
    void start();
    void resize(int rows, int columns);
    Screen& screen() { return screen_; }
signals:
    void finished(int code);
    void failed(const QString& error);
private:
    LaunchOptions launch_; Screen screen_; Process process_; bool started_ = false;
};
}
