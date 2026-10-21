#pragma once
#include <QByteArray>
#include <QUrl>
#include <QStringList>
#include <QVersionNumber>

namespace terlox::updates {
struct Release {
    QVersionNumber version;
    QString tag;
    QUrl page;
    QList<QUrl> downloads;
};
bool parseRelease(const QByteArray& json, const QString& platform, Release& release, QString& error);
QString releasePlatform();
}
