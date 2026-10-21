#include "Graphics.h"
#include <QBuffer>
#include <QImageReader>
namespace terlox {
namespace { constexpr qsizetype maxBytes=24*1024*1024; constexpr qint64 maxPixels=4*1024*1024; }
void Graphics::clear() { images_.clear(); placements_.clear(); upload_.clear(); uploadOptions_.clear(); ++revision_; }
QImage Graphics::decode(const QByteArray& data,int format,int width,int height) {
    if (data.size()>maxBytes) return {};
    if (format==100) {
        QBuffer buffer; buffer.setData(data); buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer,"PNG"); const auto size=reader.size();
        if (!size.isValid() || qint64(size.width())*size.height()>maxPixels) return {};
        return reader.read();
    }
    const int channels=format==24?3:format==32?4:0;
    if (!channels || width<=0 || height<=0 || qint64(width)*height>maxPixels || qint64(width)*height*channels!=data.size()) return {};
    return QImage(reinterpret_cast<const uchar*>(data.constData()),width,height,width*channels,channels==3?QImage::Format_RGB888:QImage::Format_RGBA8888).copy();
}
void Graphics::place(const QImage& image,QPoint cursor,qint64 scroll,int id,int columns,int rows) {
    qint64 bytes=image.sizeInBytes(); for (const auto& placement:placements_) bytes+=placement.image.sizeInBytes();
    while (!placements_.isEmpty() && bytes>32*1024*1024) { bytes-=placements_.first().image.sizeInBytes(); placements_.removeFirst(); }
    if (placements_.size()>=64) placements_.removeFirst();
    placements_.append({image,cursor,scroll+cursor.y(),columns,rows,id}); ++revision_;
}
bool Graphics::kitty(const QByteArray& data,QPoint cursor,qint64 scroll,const std::function<void(const QByteArray&)>& reply) {
    if (!data.startsWith('G')) return false;
    const auto split=data.indexOf(';'); const auto header=data.mid(1,split<0?-1:split-1);
    QHash<QByteArray,QByteArray> options;
    for (const auto& field:header.split(',')) { const auto equals=field.indexOf('='); if (equals>0) options[field.left(equals)]=field.mid(equals+1); }
    const auto action=options.value("a","t"); const int id=options.value("i").toInt();
    auto response=[&](const QByteArray& text) { const int quiet=options.value("q").toInt(); if (quiet==2 || (quiet==1 && text=="OK")) return; reply("\033_Gi="+QByteArray::number(options.value("i").toInt())+";"+text+"\033\\"); };
    if (action=="d") {
        if (!id) clear(); else { images_.remove(id); for (qsizetype i=placements_.size();i-->0;) if (placements_[i].id==id) placements_.removeAt(i); ++revision_; }
        response("OK"); return true;
    }
    if (action=="p") { if (!images_.contains(id)) response("ENOENT: image unavailable"); else { place(images_[id],cursor,scroll,id,options.value("c").toInt(),options.value("r").toInt()); response("OK"); } return true; }
    if (action!="t" && action!="T" && action!="q") { response("ENOTSUP: animation action unsupported"); return true; }
    if ((!options.value("t").isEmpty() && options.value("t")!="d") || options.contains("o")) { response("ENOTSUP: use direct uncompressed PNG/RGB/RGBA data"); return true; }
    if (upload_.isEmpty()) uploadOptions_=options;
    if (split>=0) upload_+=data.mid(split+1);
    if (upload_.size()>maxBytes) { upload_.clear(); uploadOptions_.clear(); response("E2BIG: image upload limit"); return true; }
    if (options.value("m")=="1") return true;
    const auto complete=uploadOptions_; options=complete;
    const auto bytes=QByteArray::fromBase64(upload_); upload_.clear(); uploadOptions_.clear();
    const auto image=decode(bytes,complete.value("f","32").toInt(),complete.value("s").toInt(),complete.value("v").toInt());
    if (image.isNull()) { response("EINVAL: invalid/oversized image"); return true; }
    if (complete.value("a")=="q") { response("OK"); return true; }
    const auto imageId=complete.value("i").toInt()>0?complete.value("i").toInt():nextId_++;
    qint64 total=image.sizeInBytes(); for (const auto& stored:images_) total+=stored.sizeInBytes();
    if (total>32*1024*1024 || images_.size()>=64) { images_.clear(); placements_.clear(); ++revision_; }
    images_[imageId]=image;
    if (complete.value("a")=="T") place(image,cursor,scroll,imageId,complete.value("c").toInt(),complete.value("r").toInt());
    response("OK"); return true;
}
bool Graphics::inlineImage(const QByteArray& data,QPoint cursor,qint64 scroll) {
    if (!data.startsWith("File=")) return false;
    const auto colon=data.indexOf(':'); if (colon<0 || !data.left(colon).contains("inline=1")) return true;
    const auto image=decode(QByteArray::fromBase64(data.mid(colon+1)),100,0,0); if (image.isNull()) return true;
    int columns=0,rows=0;
    for (const auto& field:data.left(colon).split(';')) { if (field.startsWith("width=")) columns=field.mid(6).toInt(); if (field.startsWith("height=")) rows=field.mid(7).toInt(); }
    if (images_.size()>=2) clear(); // Bound inline-image storage as well.
    const auto id=nextId_++; images_[id]=image; place(image,cursor,scroll,id,columns,rows); return true;
}
}
