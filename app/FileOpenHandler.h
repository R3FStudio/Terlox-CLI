#pragma once
#include <QObject>
namespace terlox {
// Launch Services sends .command files through QFileOpenEvent on macOS.
class FileOpenHandler:public QObject {
public:
    explicit FileOpenHandler(QObject* parent=nullptr):QObject(parent) {}
protected:
    bool eventFilter(QObject*,QEvent*) override;
};
}
