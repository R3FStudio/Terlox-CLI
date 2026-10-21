#include "Session.h"
namespace terlox {
Session::Session(LaunchOptions launch, int scrollback, QObject* parent)
    : QObject(parent), launch_(std::move(launch)), screen_(launch_.rows, launch_.columns, scrollback), process_() {
    connect(&process_, &Process::received, &screen_, &Screen::feed);
    connect(&screen_, &Screen::output, &process_, &Process::write);
    connect(&process_, &Process::exited, this, &Session::finished);
}
void Session::start() { QString error; started_ = process_.start(launch_, error); if (!started_) { screen_.feed(("\r\nTerlox: " + error + "\r\n").toUtf8()); emit failed(error); } }
void Session::resize(int rows, int columns) { launch_.rows = rows; launch_.columns = columns; screen_.resize(rows, columns); if (started_) process_.resize(rows, columns); }
}
