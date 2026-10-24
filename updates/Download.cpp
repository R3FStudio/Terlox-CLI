#include "Download.h"
#include <QCryptographicHash>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTimer>

namespace terlox::updates {
static bool downloadHost(const QUrl& url) {
    return url.scheme()=="https" && url.userInfo().isEmpty() && url.port(-1)==-1
        && QStringList{"github.com","release-assets.githubusercontent.com","objects.githubusercontent.com"}.contains(url.host().toLower());
}
bool downloadPackage(const QUrl& url,const QString& digest,qint64 size,const QString& destination,const Progress& progress,QString& error) {
    error.clear();
    // Local HTTP is used by the in-process transport tests; release parsing only accepts GitHub HTTPS assets.
    const bool local=url.scheme()=="http" && url.host()=="127.0.0.1";
    if ((!downloadHost(url) && !local) || !QRegularExpression("^sha256:[a-fA-F0-9]{64}$").match(digest).hasMatch()
        || size<=0 || size>1024LL*1024*1024) { error="The release is missing valid package size or SHA-256 metadata. Use the release page."; return false; }
    QSaveFile file(destination);
    if (!file.open(QIODevice::WriteOnly)) { error=file.errorString(); return false; }
    QNetworkAccessManager network; QNetworkRequest request(url);
    request.setTransferTimeout(30000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::UserVerifiedRedirectPolicy);
    auto* reply=network.get(request); reply->setReadBufferSize(256*1024);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    qint64 received=0; int percent=-1; QEventLoop loop; QTimer timeout;
    timeout.setSingleShot(true);
    auto fail=[&](const QString& message) { if (error.isEmpty()) error=message; reply->abort(); };
    auto read=[&] {
        if (!error.isEmpty()) return;
        const auto bytes=reply->readAll(); received+=bytes.size();
        if (received>size) { fail("The downloaded package was larger than its release metadata."); return; }
        if (file.write(bytes)!=bytes.size()) { fail(file.errorString()); return; }
        hash.addData(bytes);
        const int next=int(received*100/size);
        if (next!=percent) { percent=next; if (progress) progress(QString("Downloading update: %1%").arg(percent)); }
    };
    QObject::connect(reply,&QNetworkReply::readyRead,&loop,read);
    QObject::connect(reply,&QNetworkReply::redirected,&loop,[&](const QUrl& redirect) {
        if (!downloadHost(redirect)) fail("The download redirected outside GitHub's release servers.");
        else reply->redirectAllowed();
    });
    QObject::connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);
    QObject::connect(&timeout,&QTimer::timeout,&loop,[&] { fail("The update download timed out. Your installation is unchanged."); });
    timeout.start(10*60*1000); loop.exec(); timeout.stop(); read();
    if (!error.isEmpty()) return false;
    if (reply->error()!=QNetworkReply::NoError || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200) { error="Download failed: "+reply->errorString(); return false; }
    if (received!=size || hash.result().toHex()!=digest.mid(7).toLower().toLatin1()) { error="The package checksum or size did not match. Nothing was installed."; return false; }
    if (!file.commit()) { error=file.errorString(); return false; }
    return true;
}
}
