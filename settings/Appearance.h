#pragma once
#include <QColor>
#include <QFont>
namespace terlox {
struct Appearance { QFont font; double opacity = .45, blur = .85; bool effects = true; QColor border{"#ba5666"}; int scrollback = 10000; Appearance(); };
}
