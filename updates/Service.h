#pragma once
#include <QLocalServer>
namespace terlox::updates {
class Service : public QObject {
    Q_OBJECT
public:
    explicit Service(QObject* parent=nullptr);
signals:
    void restartRequested();
private:
    void install(QLocalSocket* socket);
    QLocalServer server_;
    QByteArray token_;
    bool busy_=false;
};
bool requestInstallation(QString& error);
}
