#include "FileOpenHandler.h"
#include "window/Window.h"
#include <QFileOpenEvent>
#include <QFileInfo>
#include <QMessageBox>
namespace terlox {
bool FileOpenHandler::eventFilter(QObject*,QEvent* event) {
    if (event->type()!=QEvent::FileOpen) return false;
    const auto* opened=static_cast<QFileOpenEvent*>(event);
    const QFileInfo file(opened->file());
    if (file.suffix()!="command" || !file.isFile() || !file.isExecutable()) {
        QMessageBox::warning(nullptr,"Cannot run terminal script","Choose an executable .command file. Terlox does not change script permissions automatically."); return true;
    }
    Profile profile{file.fileName(),{file.absoluteFilePath(),{},file.absolutePath()}};
    auto* window=new Window({},nullptr,&profile); window->setAttribute(Qt::WA_DeleteOnClose); window->show();
    return true;
}
}
