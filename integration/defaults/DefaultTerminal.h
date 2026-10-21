#pragma once
#include <QString>
namespace terlox {
struct DefaultTerminalResult { bool success; QString message; };
// Preferences apply to the current account. No implicit elevation.
bool supportsDefaultTerminal();
QString defaultTerminalDetails();
DefaultTerminalResult selectDefaultTerminal(const QString& executable);
DefaultTerminalResult defaultTerminalStatus();
DefaultTerminalResult restoreDefaultTerminal();
}
