#pragma once
#include <QString>
#include <QtGlobal>
namespace terlox {
// Owned handles received from Windows COM, never parsed from a command line.
struct InboundSession {
    void *input=nullptr,*output=nullptr,*signal=nullptr,*reference=nullptr,*server=nullptr,*client=nullptr;
    QString title;
    InboundSession()=default;
    InboundSession(const InboundSession&)=delete;
    InboundSession& operator=(const InboundSession&)=delete;
    ~InboundSession();
};
}
