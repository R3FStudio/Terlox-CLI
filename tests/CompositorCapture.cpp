#include "CompositorCapture.h"
#include <QWidget>
#include <windows.h>
#include <QtTest>
#include <thread>
#include <atomic>
namespace terlox::testing {
void doubleClickOnDesktop(QWidget* target,const QPoint& point) {
    const auto native=reinterpret_cast<HWND>(target->window()->winId());
    const auto offset=target->mapTo(target->window(),point);
    POINT position{LONG(offset.x()*target->devicePixelRatioF()),LONG(offset.y()*target->devicePixelRatioF())}; ClientToScreen(native,&position);
    POINT previous{}; GetCursorPos(&previous); std::atomic<bool> done=false;
    std::thread input([&] {
        SetForegroundWindow(native); SetCursorPos(position.x,position.y); Sleep(100);
        INPUT button{}; button.type=INPUT_MOUSE;
        for (int i=0;i<2;++i) { button.mi.dwFlags=MOUSEEVENTF_LEFTDOWN; SendInput(1,&button,sizeof(button)); Sleep(40);
            button.mi.dwFlags=MOUSEEVENTF_LEFTUP; SendInput(1,&button,sizeof(button)); Sleep(80); }
        SetCursorPos(previous.x,previous.y); done=true;
    });
    while (!done) QTest::qWait(10); input.join(); QTest::qWait(80);
}
void dragOnDesktop(QWidget* target, const QPoint& localStart, const QPoint& delta, bool alt, bool* cursorUnchanged) {
    const auto native=reinterpret_cast<HWND>(target->window()->winId());
    const auto offset=target->mapTo(target->window(),localStart);
    POINT start{LONG(offset.x()*target->devicePixelRatioF()),LONG(offset.y()*target->devicePixelRatioF())}; ClientToScreen(native,&start);
    POINT old{}; GetCursorPos(&old);
    std::atomic<bool> done=false;
    bool stable=true;
    std::thread input([&] {
        SetForegroundWindow(native); SetCursorPos(start.x,start.y); Sleep(100);
        CURSORINFO initial{sizeof(CURSORINFO)}; GetCursorInfo(&initial);
        INPUT modifier{}; modifier.type=INPUT_KEYBOARD; modifier.ki.wVk=VK_MENU;
        if (alt) SendInput(1,&modifier,sizeof(modifier));
        INPUT button{}; button.type=INPUT_MOUSE; button.mi.dwFlags=MOUSEEVENTF_LEFTDOWN; SendInput(1,&button,sizeof(button)); Sleep(100);
        for (int i=1;i<=15;++i) {
            SetCursorPos(start.x+delta.x()*i/15,start.y+delta.y()*i/15); Sleep(50);
            CURSORINFO current{sizeof(CURSORINFO)};
            if (GetCursorInfo(&current) && current.hCursor!=initial.hCursor) stable=false;
        }
        button.mi.dwFlags=MOUSEEVENTF_LEFTUP; SendInput(1,&button,sizeof(button)); Sleep(80);
        if (alt) { modifier.ki.dwFlags=KEYEVENTF_KEYUP; SendInput(1,&modifier,sizeof(modifier)); }
        SetCursorPos(old.x,old.y); done=true;
    });
    while (!done) QTest::qWait(10);
    input.join(); QTest::qWait(80);
    if (cursorUnchanged) *cursorUnchanged=stable;
}
void showOnDesktop(QWidget* window) {
    const auto hwnd=reinterpret_cast<HWND>(window->winId());
    ShowWindow(hwnd,SW_SHOWNORMAL);
    SetWindowPos(hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
    window->raise(); window->activateWindow();
}
QImage captureDesktopWindow(QWidget* window) {
    RECT rect{}; if (!GetWindowRect(reinterpret_cast<HWND>(window->winId()),&rect)) return {};
    const int width=rect.right-rect.left,height=rect.bottom-rect.top;
    if (width<=0 || height<=0) return {};
    HDC desktop=GetDC(nullptr),memory=CreateCompatibleDC(desktop);
    BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth=width;
    info.bmiHeader.biHeight=-height; info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    void* pixels=nullptr; HBITMAP bitmap=CreateDIBSection(desktop,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
    if (!bitmap) { DeleteDC(memory); ReleaseDC(nullptr,desktop); return {}; }
    HGDIOBJ old=SelectObject(memory,bitmap);
    const bool copied=BitBlt(memory,0,0,width,height,desktop,rect.left,rect.top,SRCCOPY|CAPTUREBLT);
    QImage image;
    if (copied) image=QImage(static_cast<uchar*>(pixels),width,height,width*4,QImage::Format_RGB32).copy();
    SelectObject(memory,old); DeleteObject(bitmap); DeleteDC(memory); ReleaseDC(nullptr,desktop); return image;
}
}
