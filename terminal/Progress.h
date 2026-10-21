#pragma once
#include <QByteArray>
namespace terlox {
struct TerminalProgress {
    // OSC 9;4: none, normal, error, indeterminate, paused.
    int state=0, percent=0;
    bool parse(const QByteArray& payload);
};
}
