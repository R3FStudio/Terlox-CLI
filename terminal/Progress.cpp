#include "Progress.h"
#include <QList>
#include <algorithm>
namespace terlox {
bool TerminalProgress::parse(const QByteArray& payload) {
    const auto fields=payload.split(';');
    if (fields.size()<2 || fields[0]!="4") return false;
    bool valid=false; const int next=fields[1].toInt(&valid);
    if (!valid || next<0 || next>4) return false;
    int value=0;
    if (fields.size()>2 && !fields[2].isEmpty()) {
        value=fields[2].toInt(&valid); if (!valid) return false;
    }
    state=next; percent=next==0?0:std::clamp(value,0,100); return true;
}
}
