#pragma once
#include <QImage>
class QWidget;
namespace terlox::testing { QImage captureDesktopWindow(QWidget* window); }
namespace terlox::testing { void showOnDesktop(QWidget* window); }
namespace terlox::testing { void doubleClickOnDesktop(QWidget* target,const QPoint& point); }
namespace terlox::testing { void dragOnDesktop(QWidget* target, const QPoint& localStart, const QPoint& delta, bool alt=true, bool* cursorUnchanged=nullptr); }
