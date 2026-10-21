#include <QtGui/qwindowdefs.h>
#import <AppKit/AppKit.h>
namespace terlox {
void activateMacWindow(WId id) {
    NSView* view=reinterpret_cast<NSView*>(id);
    if (!view.window) return;
    [NSApp activateIgnoringOtherApps:YES];
    [view.window makeKeyAndOrderFront:nil];
}
}
