#include "platform/Backdrop.h"
#include <QGuiApplication>
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
namespace terlox {
void applyWindowBorder(WId, const QColor&) {}
bool applyBackdrop(WId id, double blur, bool effects) {
    if (QGuiApplication::platformName()!="cocoa") return false;
    NSView* view=reinterpret_cast<NSView*>(id);
    if (!view || !view.window) return false;
    NSWindow* window=view.window; window.opaque=NO; window.backgroundColor=NSColor.clearColor;
    NSArray* previous=[view.subviews copy];
    for (NSView* child in previous) if ([child.identifier isEqualToString:@"terloxBackdrop"]) [child removeFromSuperview];
    [previous release];
    if (blur<=0) return false;
    NSVisualEffectView* effect=[[NSVisualEffectView alloc] initWithFrame:view.bounds];
    effect.identifier=@"terloxBackdrop"; effect.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;
    effect.material=effects?NSVisualEffectMaterialHUDWindow:NSVisualEffectMaterialUnderWindowBackground; effect.blendingMode=NSVisualEffectBlendingModeBehindWindow;
    effect.alphaValue=blur;
    effect.state=NSVisualEffectStateFollowsWindowActiveState; effect.wantsLayer=YES;
    effect.layer.cornerRadius=9.; effect.layer.masksToBounds=YES;
    [view addSubview:effect positioned:NSWindowBelow relativeTo:nil];
    [effect release]; return true;
}
}
