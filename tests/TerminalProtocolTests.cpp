#include "terminal/Screen.h"
#include <QtTest>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonArray>
using namespace terlox;
class TerminalProtocolTests : public QObject {
    Q_OBJECT
private slots:
    void editableInputClipsSelectionToCommand() {
        Screen screen(4,30);
        auto state=[&](QJsonObject object) { screen.feed("\033]777;terlox-input;"+QJsonDocument(object).toJson(QJsonDocument::Compact).toBase64()+"\033\\"); };
        screen.feed("prior output\r\ncwd> abcdef");
        state({{"active",true},{"text","abcdef"},{"widths",QJsonArray{1,1,1,1,1,1}},{"cursor",6}});
        auto deletion=screen.editableInput().selectedRange({0,0},{29,3},30,0);
        QVERIFY(deletion); QCOMPARE(deletion->first,0); QCOMPARE(deletion->count,6); QVERIFY(deletion->remaining.isEmpty());
        QVERIFY(!screen.editableInput().selectedRange({0,1},{4,1},30,0));
        QVERIFY(!screen.editableInput().selectedRange({0,0},{29,0},30,0));
        deletion=screen.editableInput().selectedRange({7,1},{9,1},30,0);
        QVERIFY(deletion); QCOMPARE(deletion->first,2); QCOMPARE(deletion->count,3); QCOMPARE(deletion->remaining,QString("abf"));
        QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); screen.key(&enter);
        QVERIFY(!screen.editableInput().selectedRange({0,0},{29,3},30,0));
        state({{"active",true},{"text","abcdef"},{"widths",QJsonArray{1}},{"cursor",6}});
        QVERIFY(!screen.editableInput().selectedRange({0,0},{29,3},30,0));
    }
    void editableInputUnicodeAndWrappedPositions() {
        EditableInput input;
        input.update({{"active",true},{"text",QString::fromUtf8("A\xe7\x95\x8c" "e\xcc\x81")},{"widths",QJsonArray{1,2,1,0}},{"cursor",4}},1,3,10,5);
        auto deletion=input.selectedRange({0,1},{1,1},10,5);
        QVERIFY(deletion); QCOMPARE(deletion->count,1); QCOMPARE(deletion->remaining,QString::fromUtf8("Ae\xcc\x81"));
        deletion=input.selectedRange({2,1},{2,1},10,5);
        QVERIFY(deletion); QCOMPARE(deletion->count,2); QCOMPARE(deletion->remaining,QString::fromUtf8("A\xe7\x95\x8c"));
        input.update({{"active",true},{"text","abcdefgh"},{"widths",QJsonArray{1,1,1,1,1,1,1,1}},{"cursor",8}},2,3,10,5);
        deletion=input.selectedRange({8,1},{1,2},10,5);
        QVERIFY(deletion); QCOMPARE(deletion->first,3); QCOMPARE(deletion->count,4); QCOMPARE(deletion->remaining,QString("abch"));
    }
    void erasedCellsKeepBackgroundAndOpacity() {
        Screen screen(3,40);
        screen.feed("\033[1;38;2;220;230;240;48;2;40;48;56m\033[2K");
        for (int col=0;col<40;++col) {
            const auto cell=screen.cell(0,col);
            QCOMPARE(screen.color(cell.bg),QColor(40,48,56));
            QCOMPARE(cell.attrs.background_alpha,255u);
            QCOMPARE(cell.attrs.foreground_alpha,255u);
            QVERIFY(!cell.attrs.bold);
        }
        screen.feed("\033]777;terlox;"+QByteArray("{\"textOpacity\":0.25,\"cellBackgroundOpacity\":0.5}").toBase64()+"\a");
        screen.feed("\033[2;1H\033[J");
        QCOMPARE(screen.cell(1,39).attrs.background_alpha,128u);
        QCOMPARE(screen.cell(2,0).attrs.foreground_alpha,64u);
        screen.feed("\033[0m\033[H\033[2J");
        QCOMPARE(screen.cell(0,0).attrs.background_alpha,255u);
    }
    void progressNotificationsAndLineRewrites() {
        Screen screen(5,60);
        for (const auto c:QByteArray("\033]9;4;1;45\033\\")) screen.feed(QByteArray(1,c));
        QCOMPARE(screen.progress().state,1); QCOMPARE(screen.progress().percent,45);
        screen.feed("\033]9;4;3;\a"); QCOMPARE(screen.progress().state,3);
        screen.feed("\033]9;4;4;120\a"); QCOMPARE(screen.progress().state,4); QCOMPARE(screen.progress().percent,100);
        screen.feed("\033]9;4;8;0\a"); QCOMPARE(screen.progress().state,4);
        screen.feed("\033]9;4;0;\a"); QCOMPARE(screen.progress().state,0);
        screen.feed(QString::fromUtf8("\rDownloading [████░░] 65%\033[K").toUtf8());
        QVERIFY(screen.text(0,0,59).contains("65%"));
        screen.feed("\rDone\033[K"); QCOMPARE(screen.text(0,0,59),QString("Done"));
    }
    void characterStylesAndAlphaSurviveHistory() {
        Screen screen(2,40);
        auto controls=[&](QByteArray json) { screen.feed("\033]777;terlox;"+json.toBase64()+"\033\\"); };
        controls("{\"textOpacity\":0.25,\"cellBackgroundOpacity\":0.5}");
        screen.feed("\033[1;2;3;4:2;38;2;18;52;86;48;5;59mA");
        auto a=screen.cell(0,0);
        QVERIFY(a.attrs.bold); QVERIFY(a.attrs.faint); QVERIFY(a.attrs.italic);
        QCOMPARE(a.attrs.underline,unsigned(VTERM_UNDERLINE_DOUBLE));
        QCOMPARE(a.attrs.foreground_alpha,64u); QCOMPARE(a.attrs.background_alpha,128u);
        QCOMPARE(screen.color(a.fg),QColor(18,52,86)); QCOMPARE(screen.color(a.bg),QColor(95,95,95));
        screen.feed("\033[22;38:2::120:140:160mB");
        auto b=screen.cell(0,1); QVERIFY(!b.attrs.bold); QVERIFY(!b.attrs.faint);
        QCOMPARE(screen.color(b.fg),QColor(120,140,160));
        // Changing the pen must not change characters already written.
        controls("{\"textOpacity\":1,\"cellBackgroundOpacity\":1}");
        screen.feed("C\033[0mD");
        QCOMPARE(screen.cell(0,0).attrs.foreground_alpha,64u);
        QCOMPARE(screen.cell(0,2).attrs.foreground_alpha,255u);
        QCOMPARE(screen.cell(0,3).attrs.foreground_alpha,255u);
        screen.feed("\r\nline two\r\nline three");
        QCOMPARE(screen.cell(0,0,1).attrs.foreground_alpha,64u);
        screen.resize(3,40); // Pop history into the visible buffer again.
        QCOMPARE(screen.cell(0,0).attrs.foreground_alpha,64u);
        controls("{\"textOpacity\":-1,\"cellBackgroundOpacity\":2}");
        screen.feed("\033[H\033[0mZ"); QCOMPARE(screen.cell(0,0).attrs.foreground_alpha,255u);
    }
    void individualLettersAndXtermPalette() {
        Screen screen(2,40);
        screen.feed("\033[38;2;255;0;0mR\033[38;2;0;255;0mG\033[38;2;0;0;255mB\033[0mW");
        QCOMPARE(screen.color(screen.cell(0,0).fg),QColor(Qt::red));
        QCOMPARE(screen.color(screen.cell(0,1).fg),QColor(Qt::green));
        QCOMPARE(screen.color(screen.cell(0,2).fg),QColor(Qt::blue));
        QCOMPARE(screen.color(screen.cell(0,3).fg),QColor(Qt::white));
        screen.feed("\033[38;5;232m0\033[38;5;255m1");
        QCOMPARE(screen.color(screen.cell(0,4).fg),QColor(8,8,8));
        QCOMPARE(screen.color(screen.cell(0,5).fg),QColor(238,238,238));
    }
    void penSaveRestoreAndStyleQuery() {
        Screen screen(3,50); QSignalSpy replies(&screen,&Screen::output);
        screen.feed("\033]777;terlox;"+QByteArray("{\"textOpacity\":0.25}").toBase64()+"\a");
        screen.feed("\033[2m\0337\033[0m\0338A");
        QCOMPARE(screen.cell(0,0).attrs.foreground_alpha,64u); QVERIFY(screen.cell(0,0).attrs.faint);
        screen.feed("\033[1;2;3;4:3;5;7;8;9;11;73m\033[38;2;1;2;3m\033[48;2;4;5;6m");
        screen.feed("\033P$qm\033\\");
        QVERIFY(!replies.isEmpty()); QVERIFY(replies.last()[0].toByteArray().startsWith("\033P1$r"));
        QVERIFY(replies.last()[0].toByteArray().contains("2;1;3;"));
    }
    void synchronizedFrames() {
        Screen screen(5,40); QSignalSpy frames(&screen,&Screen::changed);
        screen.feed("\033[?2026h\033[2J"); QCOMPARE(frames.size(),0);
        screen.feed("stable text"); QCOMPARE(frames.size(),0);
        screen.feed("\033[?2026l"); QCOMPARE(frames.size(),1); QCOMPARE(screen.text(0,0,10),QString("stable text"));
        frames.clear(); screen.feed("\033[?2026hwaiting"); QTRY_COMPARE_WITH_TIMEOUT(frames.size(),1,1000);
    }
    void rgbAndTheme() {
        Screen screen(5,40); screen.feed("\033[38;2;18;52;86m\033[48;2;120;140;160mA");
        QCOMPARE(screen.color(screen.cell(0,0).fg),QColor(18,52,86)); QCOMPARE(screen.color(screen.cell(0,0).bg),QColor(120,140,160));
        screen.feed("\033]10;#abcdef\a\033]11;rgb:12/34/56\033\\\033[0mB");
        QCOMPARE(screen.color(screen.cell(0,1).fg),QColor("#abcdef")); QCOMPARE(screen.defaultBackground(),QColor("#123456"));
        screen.feed("\033]4;201;#102030\a\033[38;5;201mC"); QCOMPARE(screen.color(screen.cell(0,2).fg),QColor("#102030"));
        QSignalSpy paletteReplies(&screen,&Screen::output);
        screen.feed("\033]4;1;#123456\a\033]4;1;?\a");
        QVERIFY(paletteReplies.last()[0].toByteArray().contains("rgb:1212/3434/5656"));
        screen.feed("\033]104;201\a"); QVERIFY(screen.color(screen.cell(0,2).fg)!=QColor("#102030"));
        QSignalSpy replies(&screen,&Screen::output); screen.feed("\033]11;?\a"); QVERIFY(replies.last()[0].toByteArray().contains("rgb:1212/3434/5656"));
        screen.feed("\033]110\a\033]111\a"); QCOMPARE(screen.defaultBackground(),QColor(Qt::black));
    }
    void fragmentedControlsAndMouse() {
        Screen screen(5,40); QSignalSpy controls(&screen,&Screen::applicationControls);
        const auto json=QByteArray("{\"opacity\":0.4,\"scale\":1.5}").toBase64();
        const auto wire="\033]777;terlox;"+json+"\033\\";
        for (const auto c:wire) screen.feed(QByteArray(1,c));
        QCOMPARE(controls.size(),1); QCOMPARE(controls[0][0].toJsonObject()["opacity"].toDouble(),.4);
        screen.feed("\033[?1000h\033[?1006h"); QVERIFY(screen.mouseReporting());
        QSignalSpy output(&screen,&Screen::output); screen.mouse({2,3},1,true,Qt::NoModifier); screen.mouse({2,3},1,false,Qt::NoModifier);
        QVERIFY(output[0][0].toByteArray().contains("<0;3;4M")); QVERIFY(output.last()[0].toByteArray().contains("<0;3;4m"));
        screen.feed("\033[?1004h"); output.clear(); screen.focus(true); QVERIFY(output.last()[0].toByteArray().contains("[I"));
    }
    void pngAndRawGraphics() {
        QImage image(8,8,QImage::Format_RGBA8888); image.fill(QColor(241,32,156));
        QByteArray png; QBuffer buffer(&png); buffer.open(QIODevice::WriteOnly); QVERIFY(image.save(&buffer,"PNG"));
        Screen screen(5,40); QSignalSpy output(&screen,&Screen::output);
        const auto encoded=png.toBase64(); const int split=encoded.size()/2;
        screen.feed("\033_Ga=T,f=100,i=7,c=4,r=2,m=1;"+encoded.left(split)+"\033\\");
        QVERIFY(screen.graphics().isEmpty());
        screen.feed("\033_Gm=0;"+encoded.mid(split)+"\033\\");
        QCOMPARE(screen.graphics().size(),1); QCOMPARE(screen.graphics()[0].image.pixelColor(0,0),QColor(241,32,156));
        QVERIFY(output.last()[0].toByteArray().contains("i=7;OK"));
        screen.feed("\033_Ga=d,i=7\033\\"); QVERIFY(screen.graphics().isEmpty());
        screen.feed("\033_Ga=T,f=24,s=1,v=1;"+QByteArray::fromHex("12abef").toBase64()+"\033\\");
        QCOMPARE(screen.graphics().size(),1); QCOMPARE(screen.graphics()[0].image.pixelColor(0,0),QColor("#12abef"));
        screen.feed("\033[2J"); QVERIFY(screen.graphics().isEmpty());
        screen.feed("\033]1337;File=inline=1;width=4;height=2:"+encoded+"\a"); QCOMPARE(screen.graphics().size(),1);
        screen.feed("\033_Ga=q,f=32,s=999999,v=999999;AAAA\033\\"); QVERIFY(output.last()[0].toByteArray().contains("EINVAL"));
    }
};
QTEST_GUILESS_MAIN(TerminalProtocolTests)
#include "TerminalProtocolTests.moc"
