#include "Shortcuts.h"
namespace terlox {
QList<Shortcut> shortcuts() {
    return {
        {Action::NewSession,QString("Ctrl+Shift+T"),"New default-shell session"},
        {Action::CloseSession,QString("Ctrl+Shift+W"),"Close current session"},
        {Action::NextSession,QString("Ctrl+Tab"),"Next session"},
        {Action::PreviousSession,QString("Ctrl+Shift+Tab"),"Previous session"},
#ifdef Q_OS_MACOS
        {Action::Copy,QString("Ctrl+C"),"Copy selected terminal text"},
        {Action::Paste,QString("Ctrl+V"),"Paste from clipboard"},
#else
        {Action::Copy,QString("Ctrl+Shift+C"),"Copy selected terminal text"},
        {Action::Paste,QString("Ctrl+Shift+V"),"Paste from clipboard"},
#endif
        {Action::LargerFont,QString("Ctrl+="),"Increase font size"},
        {Action::SmallerFont,QString("Ctrl+-"),"Decrease font size"},
        {Action::Maximize,QString("Alt+F10"),"Maximize or restore window"},
        {Action::Minimize,QString("Ctrl+Shift+M"),"Minimize window"},
        {Action::Menu,QString("Ctrl+Shift+P"),"Open terminal menu"},
        {Action::Information,QString("F1"),"In-terminal About Terlox and help"}
    };
}
}

