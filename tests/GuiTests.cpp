#include "window/Window.h"
#include "window/AnimatedMenu.h"
#include "information/Version.h"
#include "settings/Commands.h"
#include "platform/Elevation.h"
#include "app/LaunchArguments.h"
#include "platform/Process.h"
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QTabBar>
#include <QTemporaryDir>
#include <QSettings>
#include <QPlainTextEdit>
#include <QDialog>
#include <QStandardPaths>
#include <QScreen>
#include <QPainter>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QToolButton>
#include <QLabel>
#include <QScrollBar>
#include <QFontMetricsF>
#include <QFontInfo>
#ifdef Q_OS_WIN
#include "CompositorCapture.h"
#include <windows.h>
#endif
#include <QtTest>
using namespace terlox;
class GuiTests : public QObject {
    Q_OBJECT
    QTemporaryDir historyDirectory_;
    QTemporaryDir configDirectory_;
    static QString content(Screen* screen) { QStringList lines; for (int row = 0; row < screen->rows(); ++row) lines << screen->text(row,0,screen->columns()-1); return lines.join('\n'); }
private slots:
    void initTestCase() {
        QVERIFY(historyDirectory_.isValid()); QVERIFY(configDirectory_.isValid());
        qputenv("TERLOX_HISTORY_FILE",historyDirectory_.filePath("history").toUtf8());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,configDirectory_.path());
        QCoreApplication::setOrganizationName("TerloxTests"); QCoreApplication::setApplicationName("TerloxTests");
    }
    void launchFocusDoesNotPinOrRestoreMinimizedWindow() {
        if (QGuiApplication::platformName()=="offscreen") QSKIP("Needs a desktop window manager");
        QWidget other; other.resize(500,350); other.show(); other.raise(); other.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&other));
        Window terminal; terminal.show();
        QVERIFY(QTest::qWaitForWindowActive(&terminal));
        auto* view=terminal.findChild<TerminalView*>(); QVERIFY(view); QTRY_VERIFY(view->hasFocus());
        other.raise(); other.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&other));
#ifdef Q_OS_WIN
        QVERIFY(!(GetWindowLongPtrW(reinterpret_cast<HWND>(terminal.winId()),GWL_EXSTYLE)&WS_EX_TOPMOST));
        QVERIFY(GetWindowLongPtrW(reinterpret_cast<HWND>(terminal.winId()),GWL_STYLE)&WS_MINIMIZEBOX);
        // Both are ordinary windows: the newly activated one must be above it.
        auto above=GetWindow(reinterpret_cast<HWND>(other.winId()),GW_HWNDPREV);
        while (above && above!=reinterpret_cast<HWND>(terminal.winId())) above=GetWindow(above,GW_HWNDPREV);
        QVERIFY(!above);
#else
        QVERIFY(!terminal.windowFlags().testFlag(Qt::WindowStaysOnTopHint));
#endif
        terminal.showMinimized(); QTRY_VERIFY(terminal.isMinimized());
        QTest::qWait(200); QVERIFY(terminal.isMinimized());
        QVERIFY(!terminal.isActiveWindow());
    }
    void inactiveBorderAndInputFocus() {
        if (QGuiApplication::platformName()=="offscreen") QSKIP("Needs desktop activation");
        QWidget other; other.resize(300,200); other.setFocusPolicy(Qt::StrongFocus); other.show();
        for (int launch=0;launch<3;++launch) {
            other.raise(); other.activateWindow(); QVERIFY(QTest::qWaitForWindowActive(&other));
            Window terminal; terminal.show(); QVERIFY(QTest::qWaitForWindowActive(&terminal));
            auto* view=terminal.findChild<TerminalView*>(); QVERIFY(view);
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
            QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).contains('>'),10000);
            auto edge=[&] { const auto image=terminal.grab().toImage(); return image.pixelColor(image.width()/2,0); };
            QTRY_COMPARE(edge(),view->effectiveAppearance().border);
            // Use the focused widget, without clicking or assigning input focus.
            QTest::keyClicks(QApplication::focusWidget(),"echo startup-input-ok");
            QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
            QTRY_VERIFY(content(view->screen()).split('\n').contains("startup-input-ok"));
            other.raise(); other.activateWindow(); QVERIFY(QTest::qWaitForWindowActive(&other));
            QTRY_VERIFY(!terminal.isActiveWindow()); QTRY_COMPARE(edge(),QColor("#3b3b40"));
            view->applyApplicationControls({{"borderColor","#12abef"}});
            QTRY_COMPARE(edge(),QColor("#3b3b40"));
            // Reproduce a missing/restored focus target during activation.
            view->clearFocus(); terminal.raise(); terminal.activateWindow();
            QVERIFY(QTest::qWaitForWindowActive(&terminal));
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
            QTRY_COMPARE(edge(),QColor("#12abef"));
            QTest::keyClicks(QApplication::focusWidget(),"echo restored-input-ok");
            QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
            QTRY_VERIFY(content(view->screen()).split('\n').contains("restored-input-ok"));
            other.raise(); other.activateWindow(); QVERIFY(QTest::qWaitForWindowActive(&other));
            QTest::qWait(100); QVERIFY(other.isActiveWindow());
        }
    }
#ifdef Q_OS_WIN
    void nativeMinimizeRestoreKeepsFramelessClient() {
        if (QGuiApplication::platformName()=="offscreen") QSKIP("Needs Windows desktop");
        QWidget background; background.setWindowFlags(Qt::Window|Qt::FramelessWindowHint);
        background.setStyleSheet("background:black;"); background.setGeometry(100,100,1100,800); background.show();
        Window terminal; terminal.move(200,200); terminal.show(); QVERIFY(QTest::qWaitForWindowExposed(&terminal));
        const auto native=reinterpret_cast<HWND>(terminal.winId()); const auto original=terminal.size();
        for (int cycle=0;cycle<3;++cycle) {
            if (cycle==1) terminal.showMinimized();
            else SendMessageW(native,WM_SYSCOMMAND,SC_MINIMIZE,0);
            QTRY_VERIFY(terminal.isMinimized()); QTest::qWait(100);
            if (cycle==1) terminal.showNormal();
            else SendMessageW(native,WM_SYSCOMMAND,SC_RESTORE,0);
            QTRY_VERIFY(!terminal.isMinimized()); QTRY_COMPARE(terminal.size(),original);
            const auto style=GetWindowLongPtrW(native,GWL_STYLE);
            QVERIFY(!(style&WS_CAPTION)); QVERIFY(!(style&WS_MINIMIZE));
            RECT client{},frame{}; QVERIFY(GetClientRect(native,&client)); QVERIFY(GetWindowRect(native,&frame));
            POINT origin{}; QVERIFY(ClientToScreen(native,&origin));
            QCOMPARE(origin.x,frame.left); QCOMPARE(origin.y,frame.top);
            QCOMPARE(client.right,frame.right-frame.left); QCOMPARE(client.bottom,frame.bottom-frame.top);
        }
        terminal.raise(); terminal.activateWindow(); QVERIFY(QTest::qWaitForWindowActive(&terminal)); QTest::qWait(250);
        const auto captured=terlox::testing::captureDesktopWindow(&terminal); QVERIFY(!captured.isNull());
        const auto screenshot=qEnvironmentVariable("TERLOX_MINIMIZE_SCREENSHOT");
        if (!screenshot.isEmpty()) QVERIFY(captured.save(screenshot));
        const auto scale=terminal.devicePixelRatioF();
        const auto left=captured.pixelColor(qRound(60*scale),qRound(14*scale));
        const auto reference=captured.pixelColor(qRound(250*scale),qRound(14*scale));
        QVERIFY2(qAbs(left.red()-reference.red())+qAbs(left.green()-reference.green())+qAbs(left.blue()-reference.blue())<30,"Classic gray minimized caption must not remain in the blank drag strip");
    }
#endif
    void windowSessionAndTabs() {
        Window window; window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* view = window.findChild<TerminalView*>(); QVERIFY(view);
        QCOMPARE(QFontInfo(view->font()).family(),QString("Cascadia Mono"));
        QVERIFY(QFontInfo(view->font()).fixedPitch());
        const QFontMetricsF metrics(view->font());
        QVERIFY(qAbs(metrics.horizontalAdvance('W')-metrics.horizontalAdvance('i'))<.01);
        qInfo() << "Terminal font:" << QFontInfo(view->font()).family() << "advance" << metrics.horizontalAdvance('0');
#ifdef Q_OS_WIN
        QCOMPARE(QDir::cleanPath(view->property("profileDirectory").toString()),QDir::cleanPath(qEnvironmentVariable("SystemRoot","C:/Windows")+"/System32"));
#endif
        QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).contains('>'),10000);
        auto* scene=qobject_cast<SceneItem*>(view->rootObject()); QVERIFY(scene);
        QTRY_VERIFY_WITH_TIMEOUT(scene->glyphCount()>0,5000);
        QVERIFY(!view->grabFramebuffer().isNull());
        qInfo().noquote() << "Active renderer:" << view->rendererDescription();
        if (qEnvironmentVariable("QT_QUICK_BACKEND")!="software" && QGuiApplication::platformName()!="offscreen") QVERIFY(view->rendererDescription().contains("Qt RHI"));
        QVERIFY(window.windowFlags().testFlag(Qt::FramelessWindowHint));
        QVERIFY(!window.findChild<QTabBar*>()->isVisible());
        QCOMPARE(view->mapTo(&window,QPoint()).y(),32);
        const auto surface=window.grab().toImage();
        const auto header=surface.pixelColor(surface.width()/2,qRound(20*window.devicePixelRatioF()));
        const auto body=surface.pixelColor(surface.width()/2,surface.height()/2);
        QCOMPARE(header,body); // Reserved strip has exactly the same glass tint.
        const auto screenshot = qEnvironmentVariable("TERLOX_SCREENSHOT");
        if (!screenshot.isEmpty()) QVERIFY(window.grab().save(screenshot));
        view->setFocus(); QTest::keyClicks(view,"echo gui-session-ok"); QTest::keyClick(view,Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).split('\n').contains("gui-session-ok"),10000);
        QTRY_VERIFY(scene->glyphCount()>12);
        auto whitePixels=[&] {
            const auto rendered=view->grabFramebuffer(); int count=0;
            for (int y=0;y<std::min(rendered.height(),100);++y) for (int x=0;x<rendered.width();++x) { const auto pixel=rendered.pixelColor(x,y); if (pixel.red()>190 && pixel.green()>190 && pixel.blue()>190) ++count; }
            return count;
        };
        QTRY_VERIFY2(whitePixels()>200,"GPU framebuffer must contain actual terminal glyphs");
        const int oldColumns = view->screen()->columns(); window.resize(1100,700);
        QTRY_VERIFY(view->screen()->columns() > oldColumns);
        QApplication::clipboard()->setText("echo clipboard-ok"); view->paste(); QTest::keyClick(view,Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).split('\n').contains("clipboard-ok"),10000);
        // Select actual screen text with the same mouse events used by the view.
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,QPoint(0,1));
        QTest::mouseMove(view,QPoint(80,1)); QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,QPoint(80,1));
        QVERIFY(!view->selectedText().isEmpty()); view->copy(); QCOMPARE(QApplication::clipboard()->text(),view->selectedText());
        window.newDefaultSession(); QTRY_COMPARE(window.findChildren<TerminalView*>().size(),2);
        QVERIFY(window.findChild<QTabBar*>()->isVisible());
        auto* second = window.findChild<QTabWidget*>()->currentWidget();
        window.raise(); window.activateWindow(); QVERIFY(QTest::qWaitForWindowActive(&window));
        second->setFocus(); QTRY_VERIFY(second->hasFocus());
        QTest::keyClick(second,Qt::Key_W,Qt::ControlModifier|Qt::ShiftModifier);
        QTRY_COMPARE(window.findChildren<TerminalView*>().size(),1);
        view->setFocus(); QTest::keyClicks(view,"About Terlox"); QTest::keyClick(view,Qt::Key_Return);
        QTRY_VERIFY(view->aboutActive());
        QVERIFY(content(view->screen()).contains(TERLOX_VERSION));
        QTest::keyClick(view,Qt::Key_2); QVERIFY(content(view->screen()).contains("Ctrl+Shift+T"));
        QTest::keyClick(view,Qt::Key_4); QVERIFY(content(view->screen()).contains("Apache-2.0"));
        const auto aboutScreenshot=qEnvironmentVariable("TERLOX_ABOUT_SCREENSHOT");
        QTest::keyClick(view,Qt::Key_1);
        // Sample the blank left margin, away from text as the About copy changes.
        auto aboutRendered=[&] { const auto image=view->grabFramebuffer(); const auto color=image.pixelColor(1,image.height()/2); return color.blue()>=20 && color.red()<20; };
        QTRY_VERIFY(aboutRendered()); if (!aboutScreenshot.isEmpty()) QVERIFY(window.grab().save(aboutScreenshot));
        QTest::keyClick(view,Qt::Key_Escape); QVERIFY(!view->aboutActive());
        QVERIFY(content(view->screen()).contains("gui-session-ok"));
        window.close();
    }
    void draggingPreservesContent() {
        Screen screen(5,80); screen.feed("hello world"); Appearance appearance;
        TerminalView view(&screen,appearance); view.resize(640,200); view.show(); QVERIFY(QTest::qWaitForWindowExposed(&view));
        view.activateWindow(); QVERIFY(QTest::qWaitForWindowActive(&view));
        QSignalSpy menu(&view,&TerminalView::menuRequested);
        QSignalSpy output(&screen,&Screen::output);
        auto moveHeld=[&](const QPoint& position,Qt::KeyboardModifiers modifiers=Qt::NoModifier) {
            QMouseEvent event(QEvent::MouseMove,QPointF(position),QPointF(view.mapToGlobal(position)),Qt::NoButton,Qt::LeftButton,modifiers);
            QApplication::sendEvent(&view,&event);
        };
        QMouseEvent hover(QEvent::MouseMove,QPointF(20,20),QPointF(view.mapToGlobal(QPoint(20,20))),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(&view,&hover); QCOMPARE(view.cursor().shape(),Qt::IBeamCursor);
        // Clicking/reclicking prompt text focuses input and never starts a move.
        for (int i=0;i<2;++i) {
            view.clearFocus(); QTest::mouseClick(&view,Qt::LeftButton,Qt::NoModifier,QPoint(10,5));
            QVERIFY(view.hasFocus());
            QTest::keyClicks(&view,"z");
        }
        QCOMPARE(output.size(),2); QCOMPARE(output.at(0).at(0).toByteArray(),QByteArray("z"));
        QTest::mouseDClick(&view,Qt::LeftButton,Qt::NoModifier,QPoint(10,5));
        QVERIFY(view.hasFocus());
        QTest::mouseRelease(&view,Qt::LeftButton,Qt::NoModifier,QPoint(10,5));
        // Alt-drag also selects; terminal content never starts window movement.
        QTest::mousePress(&view,Qt::LeftButton,Qt::AltModifier,QPoint(10,5));
        moveHeld(QPoint(11,5),Qt::AltModifier);
        moveHeld(QPoint(70,5),Qt::AltModifier);
        moveHeld(QPoint(90,5),Qt::AltModifier);
        QTest::mouseRelease(&view,Qt::LeftButton,Qt::NoModifier,QPoint(90,5));
        QVERIFY(!view.selectedText().isEmpty());
        // Ordinary drag selects that same text without moving the window.
        QTest::mousePress(&view,Qt::LeftButton,Qt::NoModifier,QPoint(0,5));
        moveHeld(QPoint(70,5));
        QTest::mouseRelease(&view,Qt::LeftButton,Qt::ShiftModifier,QPoint(70,5));
        QVERIFY(!view.selectedText().isEmpty());
        // Right clicks over content retain the menu and keyboard focus.
        QTest::mouseClick(&view,Qt::RightButton,Qt::NoModifier,QPoint(10,5));
        QContextMenuEvent context(QContextMenuEvent::Mouse,QPoint(10,5),view.mapToGlobal(QPoint(10,5)));
        QApplication::sendEvent(&view,&context); QCOMPARE(menu.size(),1);
        QVERIFY(view.hasFocus()); QTest::keyClicks(&view,"x"); QCOMPARE(output.last().at(0).toByteArray(),QByteArray("x"));
        QCOMPARE(screen.text(0,0,10),QString("hello world"));
        screen.feed("\033[?1049h");
    }
    void animatedMenu() {
        AnimatedMenu menu; menu.addAction("Copy"); menu.addAction("Paste"); menu.popup(QPoint(100,100));
        QTRY_VERIFY(menu.isVisible()); QVERIFY(menu.windowFlags().testFlag(Qt::FramelessWindowHint)); QVERIFY(menu.windowFlags().testFlag(Qt::NoDropShadowWindowHint));
        QTRY_VERIFY(menu.windowOpacity()>.99);
        const auto path=qEnvironmentVariable("TERLOX_MENU_SCREENSHOT"); if (!path.isEmpty()) QVERIFY(menu.grab().save(path));
        menu.hide();
    }
    void smoothBorderCorners() {
        const QColor color("#ba5666"); FrameBorder border;
        for (const qreal dpr:{1.,1.25,1.5,2.}) {
            QImage image(QSize(qRound(120*dpr),qRound(80*dpr)),QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(dpr); image.fill(Qt::transparent);
            QPainter painter(&image); border.paint(painter,QSize(120,80),8,dpr,color); painter.end();
            QCOMPARE(image.pixelColor(image.width()/2,0),color);
            QCOMPARE(image.pixelColor(0,0).alpha(),0);
            bool partial=false,solid=false;
            for (int y=0;y<qRound(10*dpr);++y) for (int x=0;x<qRound(10*dpr);++x) {
                const auto pixel=image.pixelColor(x,y);
                partial|=pixel.alpha()>0 && pixel.alpha()<255; solid|=pixel.alpha()>230;
                if (pixel.alpha()>64) {
                    QVERIFY(qAbs(pixel.red()-color.red())<=2);
                    QVERIFY(qAbs(pixel.green()-color.green())<=2);
                    QVERIFY(qAbs(pixel.blue()-color.blue())<=2);
                }
            }
            QVERIFY(partial); QVERIFY(solid);
            const QColor tint(0,0,0,115);
            image.fill(tint);
            QPainter composite(&image); border.paint(composite,QSize(120,80),8,dpr,color,tint); composite.end();
            QCOMPARE(image.pixelColor(0,0).alpha(),0); // No tint left beneath the outer corner coverage.
            bool redCore=false;
            for (int y=0;y<qRound(8*dpr);++y) for (int x=0;x<qRound(8*dpr);++x) {
                const auto pixel=image.pixelColor(x,y);
                if (pixel.alpha()==255) { QCOMPARE(pixel,color); redCore=true; }
            }
            QVERIFY(redCore);
        }
    }
    void terminalLaunchArguments() {
        QStringList args{"terlox","--directory","folder with spaces","-e","python","-c","print('two words')","--profile"};
        const auto command=takeCommandArguments(args);
        QCOMPARE(args,QStringList({"terlox","--directory","folder with spaces"}));
        QCOMPARE(command,QStringList({"python","-c","print('two words')","--profile"}));
        QStringList literal{"terlox","--directory","-e","--","echo","--version"};
        QCOMPARE(takeCommandArguments(literal),QStringList({"echo","--version"}));
        QCOMPARE(literal,QStringList({"terlox","--directory","-e"}));
    }
    void historyScrollbarAndClipboard() {
        Screen screen(10,70,200); Appearance appearance; TerminalView view(&screen,appearance);
        view.resize(640,200); view.show(); QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto* bar=view.findChild<QScrollBar*>("terminalScrollBar"); QVERIFY(bar); QVERIFY(!bar->isVisible());
        for (int i=0;i<100;++i) screen.feed(QString("line-%1\r\n").arg(i,3,10,QChar('0')).toUtf8());
        QTRY_VERIFY(bar->isVisible()); QCOMPARE(bar->maximum(),screen.historySize()); QCOMPARE(bar->value(),bar->maximum());
        QWheelEvent wheel(QPointF(40,40),QPointF(view.mapToGlobal(QPoint(40,40))),QPoint(),QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(&view,&wheel);
        QTRY_VERIFY(bar->value()<bar->maximum()); QTest::qWait(170);
        QSignalSpy output(&screen,&Screen::output);
        QTest::keyClick(&view,Qt::Key_A,Qt::ControlModifier);
        QVERIFY(view.selectedText().contains("line-000")); QVERIFY(view.selectedText().contains("line-099"));
        QTest::keyClick(&view,Qt::Key_C,Qt::ControlModifier);
        QCOMPARE(QApplication::clipboard()->text(),view.selectedText()); QCOMPARE(output.size(),0);
        QApplication::clipboard()->setText("paste-proof"); QTest::keyClick(&view,Qt::Key_V,Qt::ControlModifier);
        QVERIFY(output.last().at(0).toByteArray().contains("paste-proof")); QCOMPARE(bar->value(),bar->maximum());
        output.clear(); QTest::keyClick(&view,Qt::Key_C,Qt::ControlModifier);
        QCOMPARE(output.last().at(0).toByteArray(),QByteArray(1,'\x03'));
        bar->setValue(0); const auto oldHistory=bar->maximum(); screen.feed("later-output\r\n");
        QTRY_VERIFY(bar->maximum()>oldHistory); QCOMPARE(bar->value(),0);
        const auto screenshot=qEnvironmentVariable("TERLOX_SCROLLBAR_SCREENSHOT");
        if (!screenshot.isEmpty()) QVERIFY(view.grab().save(screenshot));
        QTest::mousePress(bar,Qt::LeftButton,Qt::NoModifier,QPoint(4,12));
        QMouseEvent thumbMove(QEvent::MouseMove,QPointF(4,bar->height()-12),QPointF(bar->mapToGlobal(QPoint(4,bar->height()-12))),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(bar,&thumbMove);
        QTest::mouseRelease(bar,Qt::LeftButton,Qt::NoModifier,QPoint(4,bar->height()-12));
        QVERIFY(bar->value()>0);
        screen.feed("\033[?1049h"); QTRY_VERIFY(!bar->isVisible());
        output.clear(); QTest::keyClick(&view,Qt::Key_A,Qt::ControlModifier|Qt::AltModifier);
        QCOMPARE(output.last().at(0).toByteArray(),QByteArray(1,'\x01'));
    }
    void selectedCommandDeletionProtectsPromptAndOutput() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        Process process; Screen screen(14,110); Appearance appearance; TerminalView view(&screen,appearance);
        connect(&process,&Process::received,&screen,&Screen::feed);
        connect(&screen,&Screen::output,&process,&Process::write);
        connect(&view,&TerminalView::gridChanged,&screen,[&](int rows,int columns) { screen.resize(rows,columns); process.resize(rows,columns); });
        view.resize(1100,360); view.show(); QVERIFY(QTest::qWaitForWindowExposed(&view)); view.setFocus();
        auto executable=QCoreApplication::applicationDirPath()+"/terloxsh";
#ifdef Q_OS_WIN
        executable+=".exe";
#endif
        QString error; QVERIFY2(process.start({executable,{},directory.path(),screen.rows(),screen.columns()},error),qPrintable(error));
        QTRY_VERIFY_WITH_TIMEOUT(content(&screen).contains('>'),5000);
        const auto prompt=content(&screen).trimmed();
        auto commandLength=[&] { auto selection=screen.editableInput().selectedRange({0,-screen.historySize()},{screen.columns()-1,screen.rows()-1},screen.columns(),screen.scrollSerial()); return selection?selection->count:0; };
        for (const auto key:{Qt::Key_Backspace,Qt::Key_Delete}) {
            const auto command=QString("echo ")+QString(130,'x');
            QTest::keyClicks(&view,command); QTRY_COMPARE(commandLength(),command.size());
            QTest::keyClick(&view,Qt::Key_A,Qt::ControlModifier); QVERIFY(view.selectedText().contains('>'));
            QTest::keyClick(&view,key); QTRY_COMPARE(content(&screen).trimmed(),prompt); QTRY_COMPARE(commandLength(),0);
            QSignalSpy output(&screen,&Screen::output);
            QTest::keyClick(&view,Qt::Key_A,Qt::ControlModifier); QTest::keyClick(&view,key);
            QCOMPARE(output.size(),0); QCOMPARE(content(&screen).trimmed(),prompt);
        }
        QTest::keyClicks(&view,"echo ABCDEF"); QTRY_COMPARE(commandLength(),11);
        const auto cursor=screen.cursor(); QVERIFY(cursor.col>=3);
        const auto dpr=view.devicePixelRatioF(); const QFontMetricsF metrics(view.font());
        const auto width=std::ceil(metrics.horizontalAdvance('0')*dpr)/dpr,height=std::ceil(metrics.height()*dpr)/dpr;
        const QPoint start(qRound((cursor.col-3+.5)*width),qRound((cursor.row+.5)*height));
        const QPoint end(start.x()+qRound(2*width),start.y());
        QTest::mousePress(&view,Qt::LeftButton,Qt::NoModifier,start);
        QMouseEvent move(QEvent::MouseMove,QPointF(end),QPointF(view.mapToGlobal(end)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(&view,&move); QTest::mouseRelease(&view,Qt::LeftButton,Qt::NoModifier,end);
        QCOMPARE(view.selectedText(),QString("DEF")); QTest::keyClick(&view,Qt::Key_Delete);
        QTRY_COMPARE(commandLength(),8); QTest::keyClick(&view,Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(content(&screen).split('\n').contains("ABC"),5000);
        const auto outputBefore=content(&screen);
        QTest::keyClick(&view,Qt::Key_A,Qt::ControlModifier); QTest::keyClick(&view,Qt::Key_Backspace);
        QCOMPARE(content(&screen),outputBefore);
        QTest::keyClicks(&view,"echo still-editable"); QTest::keyClick(&view,Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(content(&screen).split('\n').contains("still-editable"),5000);
    }
    void doubleClicksBelongToApplications() {
        Window window; window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        const auto geometry=window.geometry();
        QTest::mouseDClick(&window,Qt::LeftButton,Qt::NoModifier,QPoint(200,20));
        QTest::mouseRelease(&window,Qt::LeftButton,Qt::NoModifier,QPoint(200,20));
        QVERIFY(!window.isMaximized() && !window.isFullScreen()); QCOMPARE(window.geometry(),geometry);
        Screen screen(10,80); Appearance appearance; TerminalView view(&screen,appearance);
        view.resize(700,250); view.show(); QVERIFY(QTest::qWaitForWindowExposed(&view));
        screen.feed("\033[?1000h\033[?1006h"); QSignalSpy output(&screen,&Screen::output);
        QTest::mouseDClick(&view,Qt::LeftButton,Qt::NoModifier,QPoint(20,20));
        QVERIFY(!output.isEmpty()); QVERIFY(output.last().at(0).toByteArray().startsWith("\033[<0;"));
        QTest::mouseRelease(&view,Qt::LeftButton,Qt::NoModifier,QPoint(20,20));
        QVERIFY(output.last().at(0).toByteArray().endsWith("m"));
    }
    void textZoomControls() {
        Window window; window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* view=window.findChild<TerminalView*>(); QVERIFY(view);
        QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).contains('>'),10000);
        view->setFocus();
        const auto size=view->font().pointSizeF();
        const auto columns=view->screen()->columns();
        QSignalSpy gridChanges(view,&TerminalView::gridChanged);
        auto wheel=[&](int delta,Qt::KeyboardModifiers modifiers) {
            const QPointF local(60,60);
            QWheelEvent event(local,QPointF(view->mapToGlobal(local.toPoint())),QPoint(),QPoint(0,delta),Qt::NoButton,modifiers,Qt::NoScrollPhase,false);
            QApplication::sendEvent(view,&event);
        };
        wheel(60,Qt::ControlModifier); QCOMPARE(view->font().pointSizeF(),size);
        wheel(60,Qt::ControlModifier); QCOMPARE(view->font().pointSizeF(),size+1);
        QVERIFY(!gridChanges.isEmpty());
        // Adjacent point sizes can have the same device-pixel cell width.
        wheel(360,Qt::ControlModifier); QCOMPARE(view->font().pointSizeF(),size+4);
        QTRY_VERIFY(view->screen()->columns()<columns);
        wheel(-360,Qt::ControlModifier); QCOMPARE(view->font().pointSizeF(),size+1);
        QCOMPARE(Store().appearance().font.pointSizeF(),size);
        window.executeApplicationCommand("/terlox opacity 0.46");
        QCOMPARE(Store().appearance().font.pointSizeF(),size);
        QCOMPARE(view->font().pointSizeF(),size+1);
        QTest::keyClick(view,Qt::Key_Plus,Qt::ShiftModifier); QCOMPARE(view->font().pointSizeF(),size+2);
        QTest::keyClick(view,Qt::Key_Underscore,Qt::ShiftModifier); QCOMPARE(view->font().pointSizeF(),size+1);
        QTest::keyClick(view,Qt::Key_Minus,Qt::ShiftModifier); QCOMPARE(view->font().pointSizeF(),size);
        QSignalSpy output(view->screen(),&Screen::output);
        view->screen()->feed("\033[?1049h\033[?1000h\033[?1006h");
        wheel(-120,Qt::ControlModifier); QCOMPARE(view->font().pointSizeF(),size-1);
        QCOMPARE(output.size(),0); // Zoom is consumed, never sent as application mouse input.
        wheel(120,Qt::NoModifier); QVERIFY(!output.isEmpty());
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(20,20));
        QVERIFY(output.last().at(0).toByteArray().startsWith("\033[<"));
        QTest::mousePress(view,Qt::LeftButton,Qt::ShiftModifier,QPoint(0,0));
        QMouseEvent move(QEvent::MouseMove,QPointF(80,0),QPointF(view->mapToGlobal(QPoint(80,0))),Qt::NoButton,Qt::LeftButton,Qt::ShiftModifier);
        QApplication::sendEvent(view,&move);
        QTest::mouseRelease(view,Qt::LeftButton,Qt::ShiftModifier,QPoint(80,0));
       
        view->screen()->feed("\033[?1000l\033[?1049l");
        view->openAbout("test"); wheel(120,Qt::ControlModifier);
        QCOMPARE(view->font().pointSizeF(),size); QVERIFY(view->aboutActive());
        wheel(240,Qt::ControlModifier); QCOMPARE(view->font().pointSizeF(),size+2);
        window.close();
        Window reopened; reopened.show(); QVERIFY(QTest::qWaitForWindowExposed(&reopened));
        auto* restored=reopened.findChild<TerminalView*>(); QVERIFY(restored);
        QCOMPARE(restored->font().pointSizeF(),size);
        QCOMPARE(Store().appearance().font.pointSizeF(),size);
        reopened.close();
    }
    void mouseZoomMenu() {
        Window window; window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* view=window.findChild<TerminalView*>(); QVERIFY(view);
        const auto original=view->font().pointSizeF();
        bool checked=false;
        QTimer::singleShot(180,&window,[&] {
            auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
            if (!menu) return;
            auto* plus=menu->findChild<QToolButton*>("zoomIn");
            auto* minus=menu->findChild<QToolButton*>("zoomOut");
            auto* reset=menu->findChild<QToolButton*>("zoomReset");
            auto* value=menu->findChild<QLabel*>("zoomValue");
            if (!plus || !minus || !reset || !value) { menu->close(); return; }
            QCOMPARE(value->text(),QString("100%")); QVERIFY(!reset->isEnabled());
            QTest::mouseClick(plus,Qt::LeftButton); QCOMPARE(view->font().pointSizeF(),original+1);
            QVERIFY(menu->isVisible()); QVERIFY(value->text()!="100%"); QVERIFY(reset->isEnabled());
            QTest::mouseClick(plus,Qt::LeftButton); QTest::mouseClick(minus,Qt::LeftButton);
            QCOMPARE(view->font().pointSizeF(),original+1);
            QTest::mouseClick(reset,Qt::LeftButton); QCOMPARE(view->font().pointSizeF(),original);
            QCOMPARE(value->text(),QString("100%"));
            QCOMPARE(Store().appearance().font.pointSizeF(),original);
            const auto screenshot=qEnvironmentVariable("TERLOX_ZOOM_MENU_SCREENSHOT");
            if (!screenshot.isEmpty()) QVERIFY(menu->grab().save(screenshot));
            checked=true; menu->close();
        });
        // Fail safely rather than leaving a nested popup loop active after a failed assertion.
        QTimer::singleShot(3000,&window,[] { if (auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget())) menu->close(); });
        window.showMenu(window.mapToGlobal(QPoint(100,100))); QVERIFY(checked);
        window.close();
    }
    void cliAppearanceAndGraphics() {
        Screen screen(20,80); Appearance preferences; TerminalView view(&screen,preferences); view.resize(800,400); view.show(); QVERIFY(QTest::qWaitForWindowExposed(&view));
        const auto originalFont=view.font().pointSizeF();
        const QByteArray controls="{\"opacity\":0.4,\"scale\":1.5}";
        screen.feed("\033]777;terlox;"+controls.toBase64()+"\a");
        QTRY_COMPARE(view.backgroundOpacity(),.4); QCOMPARE(view.font().pointSizeF(),originalFont*1.5);
        screen.feed("\033_Ga=T,f=24,s=1,v=1,c=8,r=4,q=2;"+QByteArray::fromHex("ff00ff").toBase64()+"\033\\");
        auto magenta=[&] { const auto frame=view.grabFramebuffer(); if (frame.isNull()) return false; const auto color=frame.pixelColor(20,20); return color.red()>220 && color.blue()>220 && color.green()<30; };
        QTRY_VERIFY2(magenta(),"CLI image pixels must be present in the GPU frame");
        screen.feed("\033]777;terlox;"+QByteArray("{\"reset\":true}").toBase64()+"\a");
        QTRY_COMPARE(view.backgroundOpacity(),preferences.opacity); QCOMPARE(view.font().pointSizeF(),originalFont);
    }
    void cliLettersKeepTheirOwnStyles() {
        Screen screen(5,60); Appearance preferences; TerminalView view(&screen,preferences);
        view.resize(800,220); view.show(); QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto* scene=qobject_cast<SceneItem*>(view.rootObject()); QVERIFY(scene);
        screen.feed("\033[?25l\033[38;2;255;0;0mR\033[38;2;0;255;0mG\033[38;2;0;0;255mB\033[0m");
        screen.feed("\033]777;terlox;"+QByteArray("{\"textOpacity\":0.25}").toBase64()+"\a");
        screen.feed("\033[2;4:2mD\033[0;4:3mU");
        screen.feed("\033]777;terlox;"+QByteArray("{\"textOpacity\":0.25}").toBase64()+"\a\033[24;38;2;255;0;0mR");
        QTRY_COMPARE(scene->glyphCount(),6);
        const auto glyphs=scene->frame().glyphs;
        QCOMPARE(glyphs[0].color,QColor(Qt::red)); QCOMPARE(glyphs[1].color,QColor(Qt::green)); QCOMPARE(glyphs[2].color,QColor(Qt::blue));
        QCOMPARE(glyphs[3].color.alpha(),32); QCOMPARE(glyphs[3].underline,2);
        QCOMPARE(glyphs[4].color.alpha(),255); QCOMPARE(glyphs[4].underline,3);
        auto peakRed=[&](int index) { const auto image=view.grabFramebuffer(); int peak=0;
            const auto rect=glyphs[index].rect; const auto scale=view.devicePixelRatioF();
            for (int y=qRound(rect.top()*scale);y<qRound(rect.bottom()*scale);++y)
                for (int x=qRound(rect.left()*scale);x<qRound(rect.right()*scale);++x)
                    { const auto color=image.pixelColor(x,y); peak=std::max(peak,(color.red()-color.green())*color.alpha()/255); }
            return peak;
        };
        QTRY_VERIFY(peakRed(0)>150);
        QTRY_VERIFY(peakRed(5)>15 && peakRed(5)<90);
        view.applyApplicationControls({{"blur",.3},{"borderColor","#12abef"},{"fontFamily","Cascadia Mono"}});
        QCOMPARE(view.effectiveAppearance().blur,.3); QCOMPARE(view.effectiveAppearance().border,QColor("#12abef"));
        view.applyApplicationControls({{"reset",true}}); QCOMPARE(view.effectiveAppearance().blur,preferences.blur);
        QSignalSpy gridChanges(&view,&TerminalView::gridChanged);
        QSignalSpy surfaceChanges(&view,&TerminalView::surfaceAppearanceChanged);
        view.applyApplicationControls({{"textOpacity",.2}});
        QCOMPARE(gridChanges.size(),0); QCOMPARE(surfaceChanges.size(),0);
    }
    void animatedStylesAndProgressFrames() {
        Screen screen(6,70); Appearance appearance; TerminalView view(&screen,appearance);
        view.resize(800,240); view.show(); QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto* scene=qobject_cast<SceneItem*>(view.rootObject()); QVERIFY(scene);
        screen.feed("\033[?25l\033[38;2;90;90;90mshimmer\r\n");
        screen.feed(QString::fromUtf8("Downloading [████░░] 65%").toUtf8());
        QTRY_VERIFY(scene->glyphCount()>20);
        auto firstColor=[&] { return scene->frame().glyphs[0].color; };
        QTRY_COMPARE(firstColor(),QColor(90,90,90));
        const auto before=view.grabFramebuffer(); QVERIFY(!before.isNull());
        screen.feed("\033[?2026h\033[H\033[1;38;2;240;240;240mshimmer\033[0m");
        // A cursor/focus repaint must not expose a partially written frame.
        view.clearFocus(); QTest::qWait(30); QCOMPARE(firstColor(),QColor(90,90,90));
        screen.feed("\033[?2026l"); QTRY_COMPARE(firstColor(),QColor(240,240,240));
        QTRY_VERIFY(view.grabFramebuffer()!=before);
        QVERIFY(std::any_of(scene->frame().glyphs.begin(),scene->frame().glyphs.end(),[](const auto& glyph) { return glyph.text==QString(QChar(0x2588)); }));
        screen.feed("\033]9;4;1;50\a");
        QTRY_VERIFY(scene->frame().fills.size()>=2);
        QTRY_COMPARE(scene->frame().fills.last().rect.width(),qreal(90));
        screen.feed("\033]9;4;3;\a");
        QTRY_COMPARE(scene->frame().fills.last().rect.width(),qreal(45));
        const auto position=scene->frame().fills.last().rect.left();
        QTRY_VERIFY(scene->frame().fills.last().rect.left()!=position);
        screen.feed("\033]9;4;0;\a"); QTRY_VERIFY(scene->frame().fills.isEmpty());
    }
    void applicationPanelsKeepExactColorsAndCellAlignment() {
        Screen screen(6,60); Appearance appearance; TerminalView view(&screen,appearance);
        view.resize(800,240); view.show(); QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto* scene=qobject_cast<SceneItem*>(view.rootObject()); QVERIFY(scene);
        screen.feed("\033[?25l\033[2;1H\033[48;2;40;48;56m\033[2K\033[49m");
        QTRY_COMPARE(scene->frame().fills.size(),60);
        const auto fills=scene->frame().fills;
        for (int col=0;col<60;++col) {
            QCOMPARE(fills[col].color,QColor(40,48,56));
            if (col) QCOMPARE(fills[col-1].rect.right(),fills[col].rect.left());
        }
        auto panelRendered=[&] {
            const auto image=view.grabFramebuffer(); const auto scale=view.devicePixelRatioF();
            const int y=qRound(fills[0].rect.center().y()*scale);
            for (int col=0;col<60;++col)
                if (image.pixelColor(qRound(fills[col].rect.center().x()*scale),y)!=QColor(40,48,56)) return false;
            return true;
        };
        QTRY_VERIFY(panelRendered());
        QCOMPARE(view.backgroundOpacity(),appearance.opacity);
    }
    void customizationCommands() {
        Appearance appearance;
        QCOMPARE(appearance.blur,.85); QVERIFY(appearance.effects);
        QVERIFY(appearanceCommand({"blur","0.25"},appearance).changed); QCOMPARE(appearance.blur,.25);
        QVERIFY(!appearanceCommand({"blur","nan"},appearance).changed); QCOMPARE(appearance.blur,.25);
        QVERIFY(!appearanceCommand({"opacity","1.1"},appearance).changed);
        QVERIFY(appearanceCommand({"font","Cascadia Mono","12"},appearance).changed); QCOMPARE(appearance.font.family(),QString("Cascadia Mono"));
        Window window; window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.raise(); window.activateWindow(); QVERIFY(QTest::qWaitForWindowActive(&window));
        auto* view=window.findChild<TerminalView*>(); QVERIFY(view);
        QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).contains('>'),10000);
        auto ready=[&] { return view->screen()->editableInput().active() && view->screen()->editableInput().empty(); };
        QTRY_VERIFY_WITH_TIMEOUT(ready(),10000);
        view->setFocus(); QTest::keyClicks(view,"/terlox blur 0.85"); QTest::keyClick(view,Qt::Key_Return);
        QTRY_COMPARE(Store().appearance().blur,.85);
        QTRY_VERIFY(content(view->screen()).contains("Updated blur"));
        QTRY_VERIFY_WITH_TIMEOUT(ready(),10000);
        QTest::keyClicks(view,"discard-input"); QTest::keyClick(view,Qt::Key_U,Qt::ControlModifier);
        QTRY_VERIFY_WITH_TIMEOUT(ready(),10000);
        QTest::keyClicks(view,"/terlox opacity 0.5"); QTest::keyClick(view,Qt::Key_Return);
        QTRY_COMPARE(Store().appearance().opacity,.5);
        QTRY_VERIFY_WITH_TIMEOUT(ready(),10000);
        QTest::keyClicks(view,"echo after-customization-ok"); QTest::keyClick(view,Qt::Key_Return);
        QTRY_VERIFY2_WITH_TIMEOUT(content(view->screen()).split('\n').contains("after-customization-ok"),qPrintable(content(view->screen())),10000);
        QStringList labels,administratorChoices;
        QTimer::singleShot(50,&window,[&] {
            auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget()); if (!menu) return;
            for (auto* action:menu->actions()) labels << action->text();
            if (auto* admin=menu->findChild<QMenu*>("administratorSessionMenu")) {
                for (auto* action:admin->actions()) administratorChoices << action->text();
            }
            menu->close();
        });
        window.showMenu(window.mapToGlobal(QPoint(100,100)));
        QVERIFY(labels.contains("New session"));
#ifdef Q_OS_WIN
        QVERIFY(labels.contains(isAdministrator()?"Administrator mode (active)":"Run as administrator"));
        if (!isAdministrator()) QCOMPARE(administratorChoices,QStringList({"PowerShell","Command Prompt"}));
#elif defined(Q_OS_MACOS)
        QVERIFY(labels.contains("Open administrator session (sudo)"));
#else
        QVERIFY(labels.contains("Open root session (sudo)"));
#endif
        QVERIFY(!labels.contains("Appearance")); QVERIFY(!labels.contains("Default shell")); QVERIFY(!labels.contains("Add shell profile..."));
        for (const auto& label:labels) QVERIFY(!label.startsWith("About") && !label.startsWith("Minimize") && !label.startsWith("Maximize") && label!="Restore");
        window.executeApplicationCommand("/terlox reset");
        QCOMPARE(Store().appearance().blur,.85); QCOMPARE(Store().appearance().opacity,.45);
        window.close();
    }
#ifdef Q_OS_WIN
    void actualDesktopDrag() {
        if (QGuiApplication::platformName()=="offscreen") QSKIP("Requires native pointer input");
        Window window; window.resize(700,450); window.move(200,200); window.show();
        terlox::testing::showOnDesktop(&window); QVERIFY(QTest::qWaitForWindowActive(&window));
        auto* view=window.findChild<TerminalView*>(); QVERIFY(view);
        QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).contains('>'),10000);
        auto* renderedScene=qobject_cast<SceneItem*>(view->rootObject()); QVERIFY(renderedScene);
        QTRY_VERIFY(renderedScene->glyphCount()>0); QVERIFY(!view->grabFramebuffer().isNull());
        QTest::qWait(250); // Let the native show/activation animation finish before coordinate-based input.
        const auto before=window.pos();
        int textRow=0;
        while (textRow<view->screen()->rows()-1 && view->screen()->text(textRow,0,view->screen()->columns()-1).isEmpty()) ++textRow;
        const int textY=qCeil(QFontMetricsF(view->font()).height())*textRow+6;
        terlox::testing::dragOnDesktop(view,QPoint(5,textY),QPoint(100,0),false);
        QCOMPARE(window.pos(),before); QVERIFY(!view->selectedText().isEmpty());
        terlox::testing::dragOnDesktop(view,QPoint(40,6),QPoint(90,60));
        QCOMPARE(window.pos(),before); // Alt no longer moves from content either.
        terlox::testing::dragOnDesktop(view,QPoint(150,150),QPoint(70,40),false);
        QCOMPARE(window.pos(),before); // Blank terminal cells remain selectable.
        bool cursorStable=false;
        terlox::testing::dragOnDesktop(&window,QPoint(200,20),QPoint(90,60),false,&cursorStable);
        QVERIFY2((window.pos()-before).manhattanLength()>30,"Only the blank top strip must move the native window");
        QVERIFY2(cursorStable,"The native cursor must stay unchanged during the entire window drag");
        QTRY_VERIFY(window.cursor().shape()!=Qt::ClosedHandCursor);
        const auto movedGeometry=window.geometry();
        terlox::testing::doubleClickOnDesktop(&window,QPoint(200,20));
        QVERIFY(!window.isMaximized() && !window.isFullScreen()); QCOMPARE(window.geometry(),movedGeometry);
        view->setFocus(); QTest::keyClicks(view,"echo drag-typing-ok"); QTest::keyClick(view,Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).split('\n').contains("drag-typing-ok"),10000);
        window.close();
    }
    void realDesktopBlur() {
        if (QGuiApplication::platformName()=="offscreen") QSKIP("Requires the native compositor");
        class Pattern : public QWidget {
        public: bool green=false;
            void paintEvent(QPaintEvent*) override { QPainter p(this); for (int x=0;x<width();x+=4) p.fillRect(x,0,4,height(),green?((x/4)%2?Qt::green:Qt::cyan):((x/4)%2?Qt::red:Qt::blue)); }
        } rear;
        rear.setWindowFlags(Qt::Window|Qt::FramelessWindowHint); rear.setGeometry(250,250,700,450); rear.show(); terlox::testing::showOnDesktop(&rear);
        Window front; front.setGeometry(rear.geometry()); front.show(); terlox::testing::showOnDesktop(&front); QVERIFY(QTest::qWaitForWindowActive(&front));
        auto contrast=[&] { const auto image=terlox::testing::captureDesktopWindow(&front); int low=255,high=0; for (int x=image.width()/2-60;x<image.width()/2+60;++x) { const auto c=image.pixelColor(x,image.height()/2); low=std::min(low,c.red()-c.blue()); high=std::max(high,c.red()-c.blue()); } return high-low; };
        front.executeApplicationCommand("/terlox blur 0"); QTest::qWait(250); const auto sharp=contrast();
        front.executeApplicationCommand("/terlox blur 0.85"); QTest::qWait(400); const auto blurred=contrast();
        qInfo() << "Actual desktop stripe contrast:" << sharp << "with blur:" << blurred;
        QVERIFY2(sharp>80,"Live rear stripes must be visible before blur");
        QVERIFY2(blurred<sharp/2,"Native blur must soften the real rear-window stripes");
        auto sample=[&] { const auto image=terlox::testing::captureDesktopWindow(&front); return image.pixelColor(image.width()/2,image.height()/2); };
        const auto purple=sample(); rear.green=true; rear.repaint();
        QTRY_VERIFY_WITH_TIMEOUT(sample().green()>purple.green()+15,5000);
        qInfo() << "Live colors through blur:" << purple << "then" << sample();
        const auto path=qEnvironmentVariable("TERLOX_BLUR_SCREENSHOT"); if (!path.isEmpty()) QVERIFY(terlox::testing::captureDesktopWindow(&front).save(path));
        front.executeApplicationCommand("/terlox blur 0"); front.close(); rear.close();
    }
    void typedWindowsCommands() {
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        const auto directory=temporary.filePath("directory with spaces"); QVERIFY(QDir().mkpath(directory));
        QFile file(directory+"/unique-listing-proof.txt"); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("file-content-proof\r\n"); file.close();
        Window window("Terlox shell"); window.resize(1400,800); window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.raise(); window.activateWindow(); QVERIFY(QTest::qWaitForWindowActive(&window));
        auto* view=window.findChild<TerminalView*>(); QVERIFY(view); view->setFocus(); QTRY_VERIFY(view->hasFocus());
        QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).contains('>'),10000);
        auto type=[&](const QString& command) { QTest::keyClicks(view,command); QTest::keyClick(view,Qt::Key_Return); };
        auto hasLine=[&](const QString& line) { return content(view->screen()).split('\n').contains(line); };
        type("cd /d \""+QDir::toNativeSeparators(directory)+"\\\"");
        QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).contains(QDir::toNativeSeparators(directory)+">"),10000);
        type("pwd"); QTRY_VERIFY_WITH_TIMEOUT(hasLine(QDir::fromNativeSeparators(directory)),10000);
        type("ls"); QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).contains("unique-listing-proof.txt"),15000);
        type("dir /b"); QTRY_VERIFY_WITH_TIMEOUT(hasLine("unique-listing-proof.txt"),10000);
        type("more unique-listing-proof.txt"); QTRY_VERIFY2_WITH_TIMEOUT(hasLine("file-content-proof"),qPrintable(content(view->screen())),10000);
        type("cd ..");
        QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).contains(QDir::toNativeSeparators(temporary.path())+">"),10000);
        type("Get-Location | Select-Object -ExpandProperty Path");
        QTRY_VERIFY_WITH_TIMEOUT(hasLine(QDir::toNativeSeparators(temporary.path())),15000);
        window.close();
    }
    void borderFollowsWindowEdges() {
        Window window; window.resize(700,450); window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.raise(); window.activateWindow(); QVERIFY(QTest::qWaitForWindowActive(&window));
        auto verifyEdges=[&] {
            const auto image=window.grab().toImage();
            const QList<QPoint> edges{{image.width()/2,0},{image.width()/2,image.height()-1},{0,image.height()/2},{image.width()-1,image.height()/2}};
            for (const auto& point:edges) {
                const auto color=image.pixelColor(point);
                if (!(color.alpha()>180 && color.red()>color.green()+40 && color.red()>color.blue()+30)) return false;
            }
            return true;
        };
        QTRY_VERIFY(verifyEdges()); window.resize(850,550); QTRY_VERIFY(verifyEdges());
        window.showMaximized();
        // With a hidden taskbar rcWork equals the monitor; Qt reports a
        // frameless maximized window as fullscreen. Both must remain restorable.
        QTRY_VERIFY(window.isMaximized() || window.isFullScreen()); QTRY_VERIFY(verifyEdges());
        if (QGuiApplication::platformName()!="offscreen") QVERIFY(window.screen()->availableGeometry().contains(window.geometry()));
        const auto corner=window.grab().toImage().pixelColor(0,0); QVERIFY(corner.alpha()>180); QVERIFY(corner.red()>corner.green()+40);
        QTest::keyClick(&window,Qt::Key_F10,Qt::AltModifier);
        QTRY_VERIFY(!window.isMaximized() && !window.isFullScreen()); QTRY_VERIFY(verifyEdges());
        window.close();
    }
    void realDesktopTransparency() {
        if (QGuiApplication::platformName()=="offscreen") QSKIP("A real desktop compositor is required");
        // These are two real, separate OS windows. Only the rear window changes.
        QWidget behind; behind.setWindowFlags(Qt::Window|Qt::FramelessWindowHint);
        behind.setAutoFillBackground(true);
        auto setBehindColor=[&](const QColor& color) { auto p=behind.palette(); p.setColor(QPalette::Window,color); behind.setPalette(p); behind.repaint(); };
        const auto available=QGuiApplication::primaryScreen()->availableGeometry();
        const QSize extent(std::min(700,available.width()-80),std::min(450,available.height()-80));
        behind.setGeometry(QRect(available.center()-QPoint(extent.width()/2,extent.height()/2),extent));
        setBehindColor(Qt::red); behind.show(); QVERIFY(QTest::qWaitForWindowExposed(&behind));
        terlox::testing::showOnDesktop(&behind);
        Window terminal; terminal.resize(extent); terminal.move(behind.pos()); terminal.show(); terminal.raise(); terminal.activateWindow();
        terlox::testing::showOnDesktop(&terminal);
        QVERIFY(QTest::qWaitForWindowExposed(&terminal)); QVERIFY(QTest::qWaitForWindowActive(&terminal));
        auto* view=terminal.findChild<TerminalView*>(); QVERIFY(view);
        QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).contains('>'),10000);
        terminal.executeApplicationCommand("/terlox blur 0");
        const auto gpu=view->grabFramebuffer(); QVERIFY(!gpu.isNull());
        const auto tintPixel=gpu.pixelColor(gpu.width()/2,gpu.height()/2);
        QCOMPARE(tintPixel.red(),0); QCOMPARE(tintPixel.green(),0); QCOMPARE(tintPixel.blue(),0);
        QVERIFY(qAbs(tintPixel.alpha()-qRound(255*view->backgroundOpacity()))<=2);
        auto sample=[&] {
            const auto capture=terlox::testing::captureDesktopWindow(&terminal);
            return capture.isNull()?QColor():capture.pixelColor(capture.width()/2,capture.height()/2);
        };
        QTRY_VERIFY_WITH_TIMEOUT(sample().red()>35 && sample().green()<15 && sample().blue()<15,5000);
        const auto red=sample(); QVERIFY(red.red()<180);
        setBehindColor(Qt::green);
        QTRY_VERIFY_WITH_TIMEOUT(sample().green()>35 && sample().red()<15 && sample().blue()<15,5000);
        const auto green=sample(); QVERIFY(green.green()<180);
        qInfo() << "Live desktop behind black tint:" << red << "then" << green;
        const auto path=qEnvironmentVariable("TERLOX_TRANSPARENCY_SCREENSHOT");
        if (!path.isEmpty()) {
            QVERIFY(terlox::testing::captureDesktopWindow(&terminal).save(path));
        }
    }
#endif
#ifdef Q_OS_WIN
    void informationInPowershell() {
        QVERIFY(!QStandardPaths::findExecutable("powershell").isEmpty());
        Window window("powershell"); window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* view=window.findChild<TerminalView*>(); QVERIFY(view);
        QTRY_VERIFY_WITH_TIMEOUT(content(view->screen()).contains("PS "),10000);
        view->setFocus(); QTest::keyClicks(view,"/info-terlox"); QTest::keyClick(view,Qt::Key_Return);
        QTRY_VERIFY(view->aboutActive());
        QTest::keyClick(view,Qt::Key_Escape); QVERIFY(!view->aboutActive());
        window.close();
    }
#endif
};
QTEST_MAIN(GuiTests)
#include "GuiTests.moc"
