#include "platform/Backdrop.h"
#include <windows.h>
#include <dwmapi.h>
namespace terlox {
void applyWindowBorder(WId id, const QColor& color) {
    Q_UNUSED(color);
    // The client draws one antialiased outline. A second DWM stroke has a
    // different corner mask and darkens/fringes the rounded joins.
    const COLORREF border=DWMWA_COLOR_NONE;
    DwmSetWindowAttribute(reinterpret_cast<HWND>(id),DWMWA_BORDER_COLOR,&border,sizeof(border));
}
bool applyBackdrop(WId id, double blur, bool effects) {
    const auto hwnd = reinterpret_cast<HWND>(id);
    const BOOL dark = TRUE; DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    const DWM_WINDOW_CORNER_PREFERENCE corners = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corners, sizeof(corners));
    // Preserve the window's alpha instead of substituting a tinted system material.
    const DWM_SYSTEMBACKDROP_TYPE backdrop = DWMSBT_NONE;
    DwmSetWindowAttribute(hwnd,DWMWA_SYSTEMBACKDROP_TYPE,&backdrop,sizeof(backdrop));
    const MARGINS glass{0,0,0,0};
    DwmExtendFrameIntoClientArea(hwnd,&glass);
    // The dynamically loaded accent policy works with Qt's layered alpha window.
    // DWM owns the live backdrop sampling and blur kernel; no screenshot is used.
    struct AccentPolicy { int state; int flags; DWORD color; int animation; };
    struct CompositionData { int attribute; void* data; SIZE_T size; };
    using SetComposition=BOOL(WINAPI*)(HWND,CompositionData*);
    const auto setComposition=reinterpret_cast<SetComposition>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"SetWindowCompositionAttribute"));
    if (!setComposition) return false;
    AccentPolicy accent{blur>0?(effects?4:3):0,0,DWORD(qRound(1+blur*63))<<24,0};
    CompositionData data{19,&accent,sizeof(accent)};
    return setComposition(hwnd,&data) && blur>0;
}
}
