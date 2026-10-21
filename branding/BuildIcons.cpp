#include "IconPainter.h"
#include <QGuiApplication>
#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QFile>
using namespace terlox;
static QByteArray png(int size) { QByteArray bytes; QBuffer output(&bytes); output.open(QIODevice::WriteOnly); renderBrandIcon(size).save(&output,"PNG"); return bytes; }
static void write(const QString& name,const QByteArray& data) { QFile file(name); if (!file.open(QIODevice::WriteOnly) || file.write(data)!=data.size()) qFatal("Cannot write icon"); }
int main(int argc,char** argv) {
    QGuiApplication app(argc,argv); if (argc!=2) return 2;
    QDir directory(QString::fromLocal8Bit(argv[1])); if (!directory.exists()) QDir().mkpath(directory.absolutePath());
    const QList<int> sizes{16,24,32,48,64,128,256,512,1024};
    for (const int size:sizes) write(directory.filePath(QString("terlox-%1.png").arg(size)),png(size));
    QByteArray ico; QDataStream out(&ico,QIODevice::WriteOnly); out.setByteOrder(QDataStream::LittleEndian);
    out<<quint16(0)<<quint16(1)<<quint16(7); quint32 offset=6+16*7; QList<QByteArray> layers;
    for (int i=0;i<7;++i) { const int size=sizes[i]; const auto data=png(size); layers<<data;
        out<<quint8(size==256?0:size)<<quint8(size==256?0:size)<<quint8(0)<<quint8(0)<<quint16(1)<<quint16(32)<<quint32(data.size())<<offset; offset+=data.size(); }
    for (const auto& layer:layers) out.writeRawData(layer.data(),layer.size()); write(directory.filePath("terlox.ico"),ico);
    QByteArray chunks; QDataStream mac(&chunks,QIODevice::WriteOnly); mac.setByteOrder(QDataStream::BigEndian);
    const QList<QPair<QByteArray,int>> types{{"icp4",16},{"icp5",32},{"icp6",64},{"ic07",128},{"ic08",256},{"ic09",512},{"ic10",1024},{"ic11",32},{"ic12",64},{"ic13",256},{"ic14",512}};
    for (const auto& type:types) { const auto data=png(type.second); mac.writeRawData(type.first.data(),4); mac<<quint32(data.size()+8); mac.writeRawData(data.data(),data.size()); }
    QByteArray icns; QDataStream header(&icns,QIODevice::WriteOnly); header.setByteOrder(QDataStream::BigEndian); header.writeRawData("icns",4); header<<quint32(chunks.size()+8); icns+=chunks; write(directory.filePath("terlox.icns"),icns);
}
