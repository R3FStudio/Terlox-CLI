#pragma once
#include "Release.h"
namespace terlox::updates {
enum class CheckState { Release, Unavailable, Error };
struct CheckResult { CheckState state=CheckState::Error; Release release; QString message; };
CheckResult checkLatest(const QUrl& endpoint=QUrl("https://api.github.com/repos/R3FStudio/Terlox-CLI/releases/latest"));
}
