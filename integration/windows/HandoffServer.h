#pragma once
#include "platform/InboundSession.h"
#include <functional>
#include <memory>
namespace terlox {
class HandoffServer {
public:
    explicit HandoffServer(std::function<void(std::shared_ptr<InboundSession>)> callback);
    ~HandoffServer();
    bool listen(QString& error);
private:
    struct Impl; std::unique_ptr<Impl> d;
};
}
