#include "platform/InboundSession.h"
#include <windows.h>
namespace terlox {
InboundSession::~InboundSession() { for (const auto handle:{input,output,signal,reference,server,client}) if (handle && handle!=INVALID_HANDLE_VALUE) CloseHandle(handle); }
}
