#pragma once
#include <QByteArray>
#include <functional>
namespace terlox {
// A bounded, incremental front end for extension sequences. Ordinary VT data
// goes unchanged to libvterm, including unknown sequences.
class Sequences {
public:
    void feed(const QByteArray& bytes, const std::function<void(const QByteArray&)>& text,
              const std::function<bool(const QByteArray&)>& extension);
private:
    enum State { Normal, Escape, Csi, String } state_=Normal;
    QByteArray pending_; bool escaped_=false, dropping_=false; char stringKind_=0;
};
}
