#include "platform/Elevation.h"
#include <QXmlStreamReader>
#include <windows.h>
namespace terlox {
bool executableRequiresElevation(const QString& executable) {
    const auto name=executable.toStdWString();
    const auto module=LoadLibraryExW(name.c_str(),nullptr,LOAD_LIBRARY_AS_DATAFILE|LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    if (!module) return false;
    auto resource=FindResourceW(module,MAKEINTRESOURCEW(1),MAKEINTRESOURCEW(24));
    if (!resource) resource=FindResourceW(module,MAKEINTRESOURCEW(2),MAKEINTRESOURCEW(24));
    bool required=false;
    if (resource) {
        const auto loaded=LoadResource(module,resource); const auto data=LockResource(loaded);
        if (data) {
            QXmlStreamReader xml(QByteArray(static_cast<const char*>(data),SizeofResource(module,resource)));
            while (!xml.atEnd()) { xml.readNext(); if (xml.isStartElement() && xml.name()==QStringLiteral("requestedExecutionLevel")) {
                required=xml.attributes().value("level")==QStringLiteral("requireAdministrator"); break;
            } }
        }
    }
    FreeLibrary(module); return required;
}
}
