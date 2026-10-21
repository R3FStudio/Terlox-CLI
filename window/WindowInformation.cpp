#include "Window.h"
#include "information/Report.h"
#include "platform/Elevation.h"
namespace terlox {
QString Window::informationText() const {
    ReportContext context;
    if (auto* view=current()) { context.renderer=view->rendererDescription(); context.profile=view->property("profileName").toString(); context.executable=view->property("profileExecutable").toString(); context.directory=view->property("profileDirectory").toString(); context.arguments=view->property("profileArguments").toStringList();
        context.surface=QString("%1 columns x %2 rows; %3 %4 pt; opacity %5%; scrollback %6 lines").arg(view->screen()->columns()).arg(view->screen()->rows()).arg(view->font().family()).arg(view->font().pointSizeF()).arg(qRound(appearance_.opacity*100)).arg(appearance_.scrollback);
    }
    context.backdrop=nativeBackdrop_?QString("Native live background blur/effects; intensity %1; black tint; OS-controlled blur kernel").arg(appearance_.blur):"Actual compositor transparency with black alpha tint; native blur disabled/unavailable";
    for (const auto& p:profiles_) context.profiles << p.name;
    return informationReport(context)+(isAdministrator()?"\nSession privilege: Windows Administrator\n":"\nSession privilege: standard user\n");
}
void Window::showInformation() { if (auto* view=current()) view->openAbout(informationText()); }
}
