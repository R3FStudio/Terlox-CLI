#include "Release.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSysInfo>

namespace terlox::updates {
static bool repositoryUrl(const QUrl& url,const QString& path) {
    return url.scheme()=="https" && url.host().compare("github.com",Qt::CaseInsensitive)==0
        && url.userInfo().isEmpty() && url.port(-1)==-1
        && url.path().startsWith("/R3FStudio/Terlox-CLI/"+path,Qt::CaseInsensitive);
}
QString releasePlatform() {
#ifdef Q_OS_WIN
    return QSysInfo::buildCpuArchitecture()=="x86_64"?"windows-x64":QString();
#elif defined(Q_OS_MACOS)
    return QSysInfo::buildCpuArchitecture()=="arm64"?"macos-arm64":
        QSysInfo::buildCpuArchitecture()=="x86_64"?"macos-x64":QString();
#else
    return QSysInfo::buildCpuArchitecture()=="x86_64"?"linux-x64":QString();
#endif
}
bool parseRelease(const QByteArray& json,const QString& platform,Release& release,QString& error) {
    release={}; error.clear();
    const auto document=QJsonDocument::fromJson(json);
    if (!document.isObject()) { error="GitHub returned an invalid release response."; return false; }
    const auto object=document.object();
    if (object.value("draft").toBool() || object.value("prerelease").toBool()) { error="This is not a stable published release."; return false; }
    const auto tag=object.value("tag_name").toString();
    const auto match=QRegularExpression("^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)(?:\\+[A-Za-z0-9.-]+)?$").match(tag);
    if (!match.hasMatch()) { error="The release tag is not a supported version number."; return false; }
    QList<int> segments;
    for (int i=1;i<=3;++i) { bool valid; const auto n=match.captured(i).toInt(&valid); if (!valid) { error="The release version is too large."; return false; } segments << n; }
    release.version=QVersionNumber(segments); release.tag=tag;
    release.page=QUrl(object.value("html_url").toString());
    if (!repositoryUrl(release.page,"releases/tag/")) { error="The release page does not belong to the Terlox repository."; return false; }
    const auto v=release.version.toString(); QStringList names;
    if (platform=="windows-x64") names << "Terlox-"+v+"-windows-x64-setup.exe";
    else if (platform=="linux-x64") names << "terlox_"+v+"_amd64.deb" << "Terlox-"+v+"-x86_64.AppImage";
    else if (platform=="macos-arm64" || platform=="macos-x64") names << "Terlox-"+v+'-'+platform+".pkg";
    for (const auto& expected:names) for (const auto& value:object.value("assets").toArray()) {
        const auto asset=value.toObject(); const auto url=QUrl(asset.value("browser_download_url").toString());
        if (asset.value("name").toString()==expected && repositoryUrl(url,"releases/download/")) { release.downloads << url; break; }
    }
    return true;
}
}
