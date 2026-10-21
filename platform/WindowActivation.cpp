#include "WindowActivation.h"
#include <QGuiApplication>
#include <QWindow>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace terlox {
#ifdef Q_OS_MACOS
void activateMacWindow(WId id);
#endif
void bringWindowToFront(QWindow* window) {
    if (!window || QGuiApplication::platformName()=="offscreen") return;
    window->raise(); window->requestActivate();
#ifdef Q_OS_WIN
    const auto native=reinterpret_cast<HWND>(window->winId());
    SetWindowPos(native,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
    if (SetForegroundWindow(native)) return;
    // Console handoff starts through COM rather than the foreground Explorer
    // process. Borrow its input queue only for this initial activation, then
    // detach immediately. Never change the topmost band or keep taking focus.
    const auto foreground=GetForegroundWindow();
    const auto foregroundThread=GetWindowThreadProcessId(foreground,nullptr);
    const auto thread=GetCurrentThreadId();
    if (!foreground || !foregroundThread || foregroundThread==thread) return;
    DWORD_PTR reply=0;
    if (!SendMessageTimeoutW(foreground,WM_NULL,0,0,SMTO_ABORTIFHUNG|SMTO_BLOCK,200,&reply)) return;
    if (AttachThreadInput(thread,foregroundThread,TRUE)) {
        BringWindowToTop(native); SetActiveWindow(native);
        AttachThreadInput(thread,foregroundThread,FALSE);
    }
#elif defined(Q_OS_MACOS)
    activateMacWindow(window->winId());
#endif
}
}
