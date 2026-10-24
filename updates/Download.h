#pragma once
#include <QUrl>
#include <QString>
#include <functional>
namespace terlox::updates {
using Progress=std::function<void(const QString&)>;
bool downloadPackage(const QUrl& url,const QString& digest,qint64 size,const QString& destination,const Progress& progress,QString& error);
}
