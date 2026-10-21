#pragma once
#include <QList>
#include <QString>
namespace terlox {
enum class Action { NewSession, CloseSession, NextSession, PreviousSession, Copy, Paste, LargerFont, SmallerFont, Maximize, Minimize, Menu, Information };
struct Shortcut { Action action; QString keys; QString description; };
QList<Shortcut> shortcuts();
}
