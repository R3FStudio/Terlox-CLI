#pragma once
#include "Scrollback.h"
#include <QObject>
#include <QColor>
#include <QKeyEvent>
#include <QJsonObject>
#include <QTimer>
#include "Sequences.h"
#include "Graphics.h"
#include "Progress.h"
#include "EditableInput.h"
namespace terlox {
class Screen : public QObject {
    Q_OBJECT
public:
    Screen(int rows, int columns, int scrollback = 10000, QObject* parent = nullptr);
    ~Screen() override;
    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;
    void feed(const QByteArray& bytes);
    void resize(int rows, int columns);
    void key(QKeyEvent* event);
    void paste(const QString& text);
    VTermScreenCell cell(int row, int column, int offset = 0) const;
    QColor color(VTermColor color) const;
    QString text(int row, int first, int last, int offset = 0) const;
    int rows() const { return rows_; } int columns() const { return columns_; }
    int historySize() const { return history_.size(); }
    VTermPos cursor() const;
    bool cursorVisible() const { return visible_; }
    bool alternate() const { return alternate_; }
    bool mouseReporting() const { return mouseMode_!=0; }
    void mouse(const QPoint& cell, int button, bool pressed, Qt::KeyboardModifiers modifiers);
    void mouseMove(const QPoint& cell, Qt::KeyboardModifiers modifiers);
    void focus(bool focused);
    QColor defaultBackground() const { return background_; }
    bool themedBackground() const { return themedBackground_; }
    bool cursorBlinks() const { return cursorBlinks_; }
    bool synchronizedUpdates() const { return synchronized_; }
    const TerminalProgress& progress() const { return progress_; }
    const EditableInput& editableInput() const { return editableInput_; }
    int cursorShape() const { return cursorShape_; }
    const QVector<Graphic>& graphics() const { return graphics_.placements(); }
    quint64 graphicRevision() const { return graphics_.revision(); }
    qint64 scrollSerial() const { return scrollSerial_; }
signals:
    void changed();
    void output(const QByteArray& bytes);
    void titleChanged(const QString& title);
    void applicationControls(const QJsonObject& controls);
private:
    VTerm* vt_; VTermScreen* screen_; Scrollback history_;
    int rows_, columns_; bool visible_ = true, alternate_ = false;
    void markChanged();
    bool extension(const QByteArray& sequence);
    bool themeSequence(int command,const QByteArray& data);
    void writeVt(const QByteArray& bytes);
    Sequences sequences_; Graphics graphics_;
    TerminalProgress progress_;
    EditableInput editableInput_;
    QColor foreground_{Qt::white},background_{Qt::black}; bool themedBackground_=false;
    QHash<int,QColor> palette_;
    bool cursorBlinks_=true; int cursorShape_=VTERM_PROP_CURSORSHAPE_BLOCK;
    int mouseMode_=0; qint64 scrollSerial_=0;
    bool feeding_=false, dirty_=false, synchronized_=false;
    QTimer syncWatchdog_;
};
QString cellText(const VTermScreenCell& cell);
}
