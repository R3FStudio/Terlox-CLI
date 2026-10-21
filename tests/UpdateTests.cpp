#include "updates/Release.h"
#include "updates/Client.h"
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
