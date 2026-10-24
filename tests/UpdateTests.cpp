#include "updates/Release.h"
#include "updates/Client.h"
#include "updates/Command.h"
#include "updates/Download.h"
#include "updates/InstallPlan.h"
#include "updates/Service.h"
#include <QCryptographicHash>
#include <QFile>
#include <QLocalSocket>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QDir>
#include <QProcess>
#include <sstream>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>
using namespace terlox::updates;
class UpdateTests : public QObject {
    Q_OBJECT
    QByteArray release(QString tag="v2.3.4") {
        QJsonArray assets;
        for (const auto& name:{QString("Terlox-2.3.4-windows-x64-setup.exe"),QString("Terlox-2.3.4-macos-arm64.pkg"),QString("terlox_2.3.4_amd64.deb"),QString("Terlox-2.3.4-x86_64.AppImage")})
            assets.append(QJsonObject{{"name",name},{"browser_download_url","https://github.com/R3FStudio/Terlox-CLI/releases/download/v2.3.4/"+name}});
        return QJsonDocument(QJsonObject{{"tag_name",tag},{"html_url","https://github.com/R3FStudio/Terlox-CLI/releases/tag/"+tag},{"draft",false},{"prerelease",false},{"assets",assets}}).toJson();
    }
private slots:
    void confirmationRequiresYes() {
        for (const auto& answer:{"no\n","\n","y\n","yes\n"," YES \n",""}) {
            std::istringstream input(answer); std::ostringstream output;
            QCOMPARE(terlox::confirmUpdate(input,output),QString(answer).trimmed().compare("yes",Qt::CaseInsensitive)==0);
            QVERIFY(output.str().find("[yes/no]")!=std::string::npos);
        }
    }
#ifndef Q_OS_WIN
    void appImageUpdateWaitsReplacesAndRestarts() {
        for (const bool valid:{true,false}) {
            QTemporaryDir directory; QVERIFY(directory.isValid());
            const auto work=directory.filePath("update work"); QVERIFY(QDir().mkpath(work));
            const auto marker=directory.filePath("restarted");
            const auto target=directory.filePath("terminal image's file.AppImage");
            QFile old(target); QVERIFY(old.open(QIODevice::WriteOnly));
            const auto oldScript=("#!/bin/sh\nprintf old > '"+marker+"'\n").toUtf8(); old.write(oldScript); old.close();
            QVERIFY(QFile::setPermissions(target,QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner));
            const auto package=directory.filePath("downloaded image");
            const auto newScript=("#!/bin/sh\nprintf new > '"+marker+"'\n").toUtf8();
            if (valid) { QFile file(package); QVERIFY(file.open(QIODevice::WriteOnly)); file.write(newScript); }
            QProcess parent; parent.start("/bin/sleep",{"1"}); QVERIFY(parent.waitForStarted());
            InstallPlan plan; plan.kind="appimage"; plan.destination=target; plan.restart=target;
            QString error;
            QVERIFY2(launchInstallation(plan,package,work,parent.processId(),error),qPrintable(error));
            QVERIFY(!QFileInfo::exists(marker));
            QVERIFY(parent.waitForFinished(5000));
            QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(marker),10000);
            QFile restarted(marker); QVERIFY(restarted.open(QIODevice::ReadOnly)); QCOMPARE(restarted.readAll(),valid?QByteArray("new"):QByteArray("old"));
            QFile result(work+"/result.json"); QVERIFY(result.open(QIODevice::ReadOnly));
            QCOMPARE(QJsonDocument::fromJson(result.readAll()).object().value("success").toBool(),valid);
            QVERIFY(old.open(QIODevice::ReadOnly)); QCOMPARE(old.readAll(),valid?newScript:oldScript);
        }
    }
#endif
    void downloadedPackagesMustMatchMetadata() {
        const QByteArray body("verified-package-content");
        const QString digest="sha256:"+QString::fromLatin1(QCryptographicHash::hash(body,QCryptographicHash::Sha256).toHex());
        for (int scenario=0;scenario<6;++scenario) {
            QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost));
            connect(&server,&QTcpServer::newConnection,this,[&] {
                auto* socket=server.nextPendingConnection();
                connect(socket,&QTcpSocket::readyRead,socket,[&,socket] {
                    socket->readAll();
                    const auto status=scenario==4?QByteArray("500 Error"):scenario==5?QByteArray("302 Found"):QByteArray("200 OK");
                    socket->write("HTTP/1.1 "+status+"\r\nConnection: close\r\nLocation: https://example.com/untrusted\r\nContent-Length: "+QByteArray::number(body.size())+"\r\n\r\n"+body);
                    socket->disconnectFromHost();
                });
                connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
            });
            QTemporaryDir directory; const auto destination=directory.filePath("package.bin");
            QFile previous(destination); QVERIFY(previous.open(QIODevice::WriteOnly)); previous.write("keep-original"); previous.close();
            QString error;
            const auto hash=scenario==1?"sha256:"+QString(64,'0'):scenario==3?QString():digest;
            const auto size=scenario==2?body.size()-1:body.size();
            const bool success=downloadPackage(QUrl("http://127.0.0.1:"+QString::number(server.serverPort())+"/package"),hash,size,destination,{},error);
            QCOMPARE(success,scenario==0);
            QVERIFY(previous.open(QIODevice::ReadOnly));
            QCOMPARE(previous.readAll(),scenario==0?body:QByteArray("keep-original"));
            if (!success) QVERIFY(!error.isEmpty());
        }
    }
    void updateServiceRejectsUnauthenticatedRequests() {
        const auto endpoint=qgetenv("TERLOX_UPDATE_ENDPOINT"),token=qgetenv("TERLOX_UPDATE_TOKEN");
        const auto restore=qScopeGuard([&] {
            for (const auto& item:{qMakePair("TERLOX_UPDATE_ENDPOINT",endpoint),qMakePair("TERLOX_UPDATE_TOKEN",token)}) {
                if (item.second.isNull()) qunsetenv(item.first); else qputenv(item.first,item.second);
            }
        });
        Service service; QSignalSpy restart(&service,&Service::restartRequested);
        QLocalSocket client; client.connectToServer(qEnvironmentVariable("TERLOX_UPDATE_ENDPOINT"));
        QVERIFY(client.waitForConnected());
        client.write("{\"action\":\"install\",\"token\":\"wrong-token\"}\n"); client.flush();
        QTRY_COMPARE(client.state(),QLocalSocket::UnconnectedState);
        QVERIFY(restart.isEmpty());
    }
    void updateClientWaitsForInstallerAcknowledgement() {
        const auto endpoint=qgetenv("TERLOX_UPDATE_ENDPOINT"),token=qgetenv("TERLOX_UPDATE_TOKEN");
        const auto restore=qScopeGuard([&] {
            for (const auto& item:{qMakePair("TERLOX_UPDATE_ENDPOINT",endpoint),qMakePair("TERLOX_UPDATE_TOKEN",token)}) {
                if (item.second.isNull()) qunsetenv(item.first); else qputenv(item.first,item.second);
            }
        });
        for (const bool success:{true,false}) {
            QLocalServer server; server.setSocketOptions(QLocalServer::UserAccessOption);
            QVERIFY(server.listen("terlox-update-test-"+QString::number(QCoreApplication::applicationPid())+(success?"-ok":"-fail")));
            qputenv("TERLOX_UPDATE_ENDPOINT",server.fullServerName().toUtf8()); qputenv("TERLOX_UPDATE_TOKEN","test-token");
            bool authenticated=false;
            connect(&server,&QLocalServer::newConnection,this,[&] {
                auto* socket=server.nextPendingConnection();
                connect(socket,&QLocalSocket::readyRead,socket,[&,socket] {
                    if (!socket->canReadLine()) return;
                    const auto request=QJsonDocument::fromJson(socket->readLine()).object();
                    authenticated=request.value("token")=="test-token" && request.value("action")=="install";
                    socket->write(QJsonDocument(QJsonObject{{"message",success?"Installer ready":"Download failed"},{"done",true},{"success",success}}).toJson(QJsonDocument::Compact)+'\n');
                    socket->flush();
                });
                connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);
            });
            QString error; QCOMPARE(requestInstallation(error),success); QVERIFY(authenticated);
            QCOMPARE(error.isEmpty(),success);
        }
    }
    void installationTypeSelectsMatchingPackage() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const auto executable=directory.filePath("terlox.exe");
        Release parsed; InstallPlan plan; QString error;
        QVERIFY(parseRelease(release(),"windows-x64",parsed,error));
        QVERIFY(!planInstallation(parsed,"windows-x64",executable,{},{},plan,error));
        QFile manifest(directory.filePath(".terlox-install.json")); QVERIFY(manifest.open(QIODevice::WriteOnly));
        manifest.write("{\"application\":\"app.terlox.terminal\"}"); manifest.close();
        QVERIFY2(planInstallation(parsed,"windows-x64",executable,{},{},plan,error),qPrintable(error));
        QCOMPARE(plan.kind,QString("windows")); QVERIFY(plan.asset.fileName().endsWith("-setup.exe"));
        QVERIFY(parseRelease(release(),"linux-x64",parsed,error));
        const auto appImage=directory.filePath("Terlox.AppImage"); QFile file(appImage); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("image"); file.close();
        QFile binary(directory.filePath("terlox")); QVERIFY(binary.open(QIODevice::WriteOnly)); binary.close();
        QVERIFY(planInstallation(parsed,"linux-x64",binary.fileName(),appImage,directory.path(),plan,error));
        QCOMPARE(plan.kind,QString("appimage")); QVERIFY(plan.asset.fileName().endsWith(".AppImage"));
    }
    void selectsNativeAssetsAndComparesNumerically() {
        Release parsed; QString error;
        QVERIFY2(parseRelease(release(),"windows-x64",parsed,error),qPrintable(error));
        QCOMPARE(parsed.version,QVersionNumber(2,3,4)); QCOMPARE(parsed.downloads.size(),1);
        QVERIFY(parsed.downloads[0].path().endsWith("-windows-x64-setup.exe"));
        QVERIFY(parseRelease(release(),"linux-x64",parsed,error)); QCOMPARE(parsed.downloads.size(),2);
        QVERIFY(parseRelease(release(),"macos-arm64",parsed,error)); QCOMPARE(parsed.downloads.size(),1);
        QVERIFY(parseRelease(release(),"unknown",parsed,error)); QVERIFY(parsed.downloads.isEmpty());
        QVERIFY(QVersionNumber(1,10,0)>QVersionNumber(1,9,0));
    }
    void rejectsMalformedOrUnpublishedReleases() {
        Release parsed; QString error;
        QVERIFY(!parseRelease("not JSON","windows-x64",parsed,error));
        QVERIFY(!parseRelease(release("v2.3.4-rc1"),"windows-x64",parsed,error));
        auto object=QJsonDocument::fromJson(release()).object(); object["prerelease"]=true;
        QVERIFY(!parseRelease(QJsonDocument(object).toJson(),"windows-x64",parsed,error));
        object["prerelease"]=false; object["html_url"]="https://example.com/releases/tag/v2.3.4";
        QVERIFY(!parseRelease(QJsonDocument(object).toJson(),"windows-x64",parsed,error));
    }
    void handlesHttpErrorsWithoutInternet() {
        for (const auto status:{200,404,429,500}) {
            QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost));
            connect(&server,&QTcpServer::newConnection,this,[&,status] {
                auto* socket=server.nextPendingConnection();
                connect(socket,&QTcpSocket::readyRead,socket,[&,socket,status] {
                    socket->readAll(); const auto body=status==200?release():QByteArray("{}");
                    socket->write("HTTP/1.1 "+QByteArray::number(status)+" Test\r\nConnection: close\r\nContent-Length: "+QByteArray::number(body.size())+"\r\n\r\n"+body);
                    socket->disconnectFromHost();
                });
                connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
            });
            const auto result=checkLatest(QUrl("http://127.0.0.1:"+QString::number(server.serverPort())+"/latest"));
            QCOMPARE(result.state,status==200?CheckState::Release:status==404?CheckState::Unavailable:CheckState::Error);
        }
    }
};
QTEST_GUILESS_MAIN(UpdateTests)
#include "UpdateTests.moc"
