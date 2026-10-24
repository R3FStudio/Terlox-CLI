#include "Window.h"
#include "ProfileDialog.h"
#include "settings/Commands.h"
#include "platform/Elevation.h"
#include <QCoreApplication>
#include <QDir>
#include <QProcess>
namespace terlox {
void Window::executeApplicationCommand(const QString& command) {
    auto* view=current(); if (!view) return;
    auto reply=[view](QString text) { view->screen()->feed(("\r\n"+text.replace("\n","\r\n")+"\r\n").toUtf8()); view->setFocus(); };
    if (command=="/admin" || command.startsWith("/admin ")) {
        const auto admin=parseAdminCommand(command.mid(6));
        if (admin.help) { reply("/admin [--powershell|--cmd] [command]: native elevated terminal. Windows requests UAC; Unix uses sudo inside the terminal."); return; }
        openAdminSession(admin.engine,admin.command);
        return;
    }
    auto args=QProcess::splitCommand(command.mid(7).trimmed());
    if (args.value(0)=="update") {
        const auto executable=QDir(QCoreApplication::applicationDirPath()).filePath(
#ifdef Q_OS_WIN
            "terloxctl.exe"
#else
            "terloxctl"
#endif
        );
        newSession({"Update",{executable,QStringList{"update"}+args.mid(1),QDir::currentPath()}});
        return;
    }
    if (args.value(0)=="default-shell") {
        if (args.size()==2) for (const auto& profile:profiles_) if (profile.name==args[1]) { store_.setDefaultProfile(profile.name); reply("Default shell set to "+profile.name+" (new sessions)."); return; }
        QStringList names; for (const auto& profile:profiles_) names << profile.name;
        reply("Use /terlox default-shell \"name\". Available: "+names.join(", ")); return;
    }
    if (args.value(0)=="profile-add" && args.size()==1) {
        if (auto profile=editProfile(this)) { store_.addProfile(*profile); profiles_.append(*profile); newSession(*profile); }
        return;
    }
    const auto result=appearanceCommand(args,appearance_);
    if (result.changed && (args.value(0)=="font" || args.value(0)=="reset")) textZoom_=0;
    if (result.changed) applyAppearance();
    reply(result.message);
    if (result.changed && appearance_.blur>0 && !nativeBackdrop_) reply("Native blur is unavailable on this desktop; real alpha transparency is active.");
}
}
