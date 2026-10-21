#pragma once
#include <QHash>
#include <QImage>
#include <QPoint>
#include <QVector>
#include <functional>
namespace terlox {
struct Graphic { QImage image; QPoint cell; qint64 line=0; int columns=0,rows=0,id=0; };
class Graphics {
public:
    bool kitty(const QByteArray& data,QPoint cursor,qint64 scroll,const std::function<void(const QByteArray&)>& reply);
    bool inlineImage(const QByteArray& data,QPoint cursor,qint64 scroll);
    const QVector<Graphic>& placements() const { return placements_; }
    quint64 revision() const { return revision_; }
    void clear();
private:
    QImage decode(const QByteArray& bytes,int format,int width,int height);
    void place(const QImage& image,QPoint cursor,qint64 scroll,int id,int columns,int rows);
    QHash<int,QImage> images_; QVector<Graphic> placements_;
    QByteArray upload_; QHash<QByteArray,QByteArray> uploadOptions_;
    quint64 revision_=0; int nextId_=1;
};
}
