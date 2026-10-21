#pragma once
#include <QStringList>
#include <memory>
namespace terlox {
struct InboundSession;
struct LaunchOptions { QString executable; QStringList arguments; QString directory; int rows = 30, columns = 100; QString nativeArguments; std::shared_ptr<InboundSession> inbound; };
}
