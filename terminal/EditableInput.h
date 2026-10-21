#pragma once
#include <QJsonObject>
#include <QPoint>
#include <QString>
#include <optional>
namespace terlox {
struct InputDeletion { int first=0,count=0; QString remaining; bool atEnd=false; };
class EditableInput {
public:
    void update(const QJsonObject& state,int row,int column,int columns,qint64 scrollSerial);
    void clear() { active_=false; }
    std::optional<InputDeletion> selectedRange(QPoint a,QPoint b,int columns,qint64 scrollSerial) const;
private:
    bool active_=false;
    QString text_;
    QVector<int> widths_;
    qint64 origin_=0;
    int columns_=0;
};
}
