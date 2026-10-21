#include "ConptyApi.h"
#include <QCoreApplication>
#include <QDir>
namespace terlox {
const ConptyApi& ConptyApi::instance() {
    static const auto api=[] {
        ConptyApi value;
        const auto path=QDir::toNativeSeparators(QCoreApplication::applicationDirPath()+"/conpty.dll").toStdWString();
        value.module=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (value.module) {
            value.create=reinterpret_cast<decltype(value.create)>(GetProcAddress(value.module,"ConptyCreatePseudoConsole"));
            value.pack=reinterpret_cast<decltype(value.pack)>(GetProcAddress(value.module,"ConptyPackPseudoConsole"));
            value.release=reinterpret_cast<decltype(value.release)>(GetProcAddress(value.module,"ConptyReleasePseudoConsole"));
            value.resize=reinterpret_cast<decltype(value.resize)>(GetProcAddress(value.module,"ConptyResizePseudoConsole"));
            value.close=reinterpret_cast<decltype(value.close)>(GetProcAddress(value.module,"ConptyClosePseudoConsole"));
        }
        return value;
    }();
    return api;
}
}
