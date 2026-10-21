#pragma once
#include <QtGui/qwindowdefs.h>
#include <QColor>
namespace terlox { bool applyBackdrop(WId window, double blur = .85, bool effects = true); void applyWindowBorder(WId window, const QColor& color); }
