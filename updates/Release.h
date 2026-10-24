#pragma once
#include <QByteArray>
#include <QUrl>
#include <QStringList>
#include <QVersionNumber>
#include <QMap>

namespace terlox::updates {
struct Release {
    QVersionNumber version;
    QString tag;
    QUrl page;
    QList<QUrl> downloads;
    QMap<QString,QString> digests;
    QMap<QString,qint64> sizes;
};
bool parseRelease(const QByteArray& json, const QString& platform, Release& release, QString& error);
QString releasePlatform();
}
