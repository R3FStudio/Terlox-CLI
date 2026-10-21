#include "Client.h"
#include "information/Version.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>

namespace terlox::updates {
CheckResult checkLatest(const QUrl& endpoint) {
    constexpr qsizetype limit=1024*1024;
    QNetworkAccessManager network; QNetworkRequest request(endpoint);
    request.setRawHeader("Accept","application/vnd.github+json");
    request.setRawHeader("User-Agent",QByteArray("Terlox/")+TERLOX_VERSION);
    request.setTransferTimeout(10000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::NoLessSafeRedirectPolicy);
    auto* reply=network.get(request); reply->setReadBufferSize(limit+1);
    QByteArray payload; bool oversized=false,timedOut=false; QEventLoop loop; QTimer deadline;
    deadline.setSingleShot(true);
    auto read=[&] { payload+=reply->readAll(); if (payload.size()>limit) { oversized=true; reply->abort(); } };
    QObject::connect(reply,&QNetworkReply::readyRead,&loop,read);
    QObject::connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);
    QObject::connect(&deadline,&QTimer::timeout,&loop,[&] { timedOut=true; reply->abort(); });
    deadline.start(12000); loop.exec(); deadline.stop(); read();
    const auto status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (timedOut) return {CheckState::Error,{},"Update check timed out. Try again when your connection is available."};
    if (oversized) return {CheckState::Error,{},"GitHub's release response was too large."};
    if (status==404) return {CheckState::Unavailable,{},"No public stable release was found. The repository may be private or may not have a release yet."};
    if (status==403 || status==429) return {CheckState::Error,{},"GitHub refused the request or its public API rate limit was reached. Try again later."};
    if (reply->error()!=QNetworkReply::NoError || status!=200)
        return {CheckState::Error,{},"Could not check GitHub for updates: "+reply->errorString()};
    Release release; QString error;
    if (!parseRelease(payload,releasePlatform(),release,error)) return {CheckState::Error,{},error};
    return {CheckState::Release,release,{}};
}
}
