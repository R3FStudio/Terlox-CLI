#include "Service.h"
#include "Client.h"
#include "Download.h"
#include "InstallPlan.h"
#include "information/Version.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QPointer>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QUuid>
#include <iostream>

namespace terlox::updates {
Service::Service(QObject* parent):QObject(parent) {
    token_=QUuid::createUuid().toByteArray(QUuid::WithoutBraces)+QUuid::createUuid().toByteArray(QUuid::WithoutBraces);
    server_.setSocketOptions(QLocalServer::UserAccessOption);
    auto name="terlox-update-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
#ifndef Q_OS_WIN
    // macOS per-user temporary paths can exceed sockaddr_un's path limit.
    name=QDir("/tmp").filePath(name);
#endif
    if (!server_.listen(name)) { qunsetenv("TERLOX_UPDATE_ENDPOINT"); qunsetenv("TERLOX_UPDATE_TOKEN"); return; }
    qputenv("TERLOX_UPDATE_ENDPOINT",server_.fullServerName().toUtf8()); qputenv("TERLOX_UPDATE_TOKEN",token_);
    connect(&server_,&QLocalServer::newConnection,this,[this] {
        while (auto* socket=server_.nextPendingConnection()) {
            connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);
            QTimer::singleShot(5000,socket,[socket] { if (!socket->property("accepted").toBool()) socket->disconnectFromServer(); });
            connect(socket,&QLocalSocket::readyRead,this,[this,socket] {
                if (socket->property("accepted").toBool()) { socket->readAll(); return; }
                if (socket->bytesAvailable()>4096) { socket->disconnectFromServer(); return; }
                if (!socket->canReadLine()) return;
                const auto request=QJsonDocument::fromJson(socket->readLine(4096)).object();
                socket->setProperty("accepted",true);
                if (request.value("token").toString().toUtf8()!=token_ || request.value("action")!="install") { socket->disconnectFromServer(); return; }
                install(socket);
            });
        }
    });
}
void Service::install(QLocalSocket* socket) {
    QPointer<QLocalSocket> client(socket);
    auto send=[client](const QString& message,bool done=false,bool success=false) {
        if (!client) return;
        client->write(QJsonDocument(QJsonObject{{"message",message},{"done",done},{"success",success}}).toJson(QJsonDocument::Compact)+'\n'); client->flush();
    };
    if (busy_) { send("An update is already in progress.",true); return; }
    busy_=true;
    auto fail=[&](const QString& error) { busy_=false; send(error,true); };
    send("Preparing the update...");
    const auto result=checkLatest();
    if (result.state!=CheckState::Release) { fail(result.message); return; }
    if (result.release.version<=QVersionNumber::fromString(TERLOX_VERSION)) { fail("There is no newer stable version to install."); return; }
    InstallPlan plan; QString error;
    if (!planInstallation(result.release,releasePlatform(),QCoreApplication::applicationFilePath(),qEnvironmentVariable("APPIMAGE"),qEnvironmentVariable("APPDIR"),plan,error)) { fail(error); return; }
    const auto root=QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/updates";
    if (!QDir().mkpath(root)) { fail("Cannot create the update download folder."); return; }
    QTemporaryDir temporary(root+"/update-XXXXXX");
    if (!temporary.isValid()) { fail("Cannot create the update download folder."); return; }
    const auto package=temporary.filePath(plan.asset.fileName());
    if (!downloadPackage(plan.asset,result.release.digests.value(plan.asset.toString()),result.release.sizes.value(plan.asset.toString()),package,
        [&](const QString& message) { send(message); },error)) { fail(error); return; }
    if (!client || client->state()!=QLocalSocket::ConnectedState) { busy_=false; return; }
    send("Download verified. Closing Terlox to install and restart...");
    if (!launchInstallation(plan,package,temporary.path(),QCoreApplication::applicationPid(),error)) { fail(error); return; }
    temporary.setAutoRemove(false);
    send("Update installer started.",true,true);
    QTimer::singleShot(300,this,[this] { emit restartRequested(); });
}
bool requestInstallation(QString& error) {
    const auto endpoint=qEnvironmentVariable("TERLOX_UPDATE_ENDPOINT");
    const auto token=qgetenv("TERLOX_UPDATE_TOKEN");
    if (endpoint.isEmpty() || token.isEmpty()) { error="Run this command inside an installed Terlox window to install and restart automatically."; return false; }
    QLocalSocket socket; socket.connectToServer(endpoint);
    if (!socket.waitForConnected(5000)) { error="Could not reach the Terlox update service. Reopen Terlox and try again."; return false; }
    socket.write(QJsonDocument(QJsonObject{{"token",QString::fromUtf8(token)},{"action","install"}}).toJson(QJsonDocument::Compact)+'\n'); socket.flush();
    QEventLoop loop; QTimer timeout; timeout.setSingleShot(true); bool success=false,done=false;
    auto read=[&] {
        while (socket.canReadLine()) {
            const auto result=QJsonDocument::fromJson(socket.readLine(8192)).object();
            const auto message=result.value("message").toString();
            std::cout << message.toUtf8().constData() << '\n' << std::flush;
            if (result.value("done").toBool()) { done=true; success=result.value("success").toBool(); if (!success) error=message; loop.quit(); }
        }
    };
    QObject::connect(&socket,&QLocalSocket::readyRead,&loop,read);
    QObject::connect(&socket,&QLocalSocket::disconnected,&loop,[&] { if (!done) { error="The update connection closed before installation was ready."; loop.quit(); } });
    QObject::connect(&timeout,&QTimer::timeout,&loop,[&] { error="The update request timed out."; loop.quit(); });
    timeout.start(12*60*1000); read(); if (!done) loop.exec();
    return success;
}
}
