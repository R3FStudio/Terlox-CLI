#include "InstallPlan.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>

namespace terlox::updates {
bool planInstallation(const Release& release,const QString& platform,const QString& executable,const QString& appImage,const QString& appDir,InstallPlan& plan,QString& error) {
    plan={}; error.clear();
    const auto directory=QFileInfo(executable).absolutePath();
    if (platform=="windows-x64") {
        QFile inventory(directory+"/.terlox-install.json");
        if (!inventory.open(QIODevice::ReadOnly) || QJsonDocument::fromJson(inventory.readAll()).object().value("application")!="app.terlox.terminal") {
            error="Install Terlox with its setup EXE before using automatic updates."; return false;
        }
        plan.kind="windows"; plan.destination=directory; plan.restart=directory+"/terlox.exe";
    } else if (platform=="linux-x64") {
        if (!appImage.isEmpty() && !appDir.isEmpty() && QFileInfo(appImage).isFile()
            && QFileInfo(executable).canonicalFilePath().startsWith(QFileInfo(appDir).canonicalFilePath()+'/')) {
            if (!QFileInfo(appImage).isWritable() || !QFileInfo(QFileInfo(appImage).absolutePath()).isWritable()) { error="The AppImage and its folder must be writable to update it."; return false; }
            plan.kind="appimage"; plan.destination=QFileInfo(appImage).canonicalFilePath(); plan.restart=plan.destination;
        } else if (QDir::cleanPath(directory)=="/opt/terlox/usr/bin") {
            if (QStandardPaths::findExecutable("pkexec").isEmpty() || !QFileInfo::exists("/usr/bin/apt-get")) { error="Automatic Debian updates need apt-get and a desktop PolicyKit authentication agent."; return false; }
            plan.kind="deb"; plan.destination="/opt/terlox"; plan.restart="/usr/bin/terlox";
        } else { error="Automatic Linux updates require the installed Debian package or a writable Terlox AppImage."; return false; }
    } else if (platform.startsWith("macos-")) {
        if (QDir::cleanPath(executable)!="/Applications/Terlox.app/Contents/MacOS/terlox") { error="Install Terlox in /Applications with its PKG before using automatic updates."; return false; }
        plan.kind="macos"; plan.destination="/Applications/Terlox.app"; plan.restart=executable;
    } else { error="Automatic updates are unavailable for this architecture."; return false; }
    for (const auto& url:release.downloads) {
        const auto name=url.fileName();
        if ((plan.kind=="windows" && name.endsWith("-setup.exe")) || (plan.kind=="deb" && name.endsWith(".deb"))
            || (plan.kind=="appimage" && name.endsWith(".AppImage")) || (plan.kind=="macos" && name.endsWith(".pkg"))) { plan.asset=url; return true; }
    }
    error="This release does not include a package for your installation type."; return false;
}
static bool save(const QString& path,const QByteArray& bytes,QString& error) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()) { error=file.errorString(); return false; }
    QFile::setPermissions(path,QFile::ReadOwner|QFile::WriteOwner); return true;
}
bool launchInstallation(const InstallPlan& plan,const QString& package,const QString& work,qint64 parentPid,QString& error) {
    const auto report=work+"/result.json";
#ifdef Q_OS_WIN
    const auto request=work+"/request.json";
    const QJsonObject data{{"application","app.terlox.terminal"},{"directory",plan.destination},{"pid",parentPid},
        {"report",report},{"restart",true}};
    if (!save(request,QJsonDocument(data).toJson(),error)) return false;
    QProcess installer; installer.setProgram(package); installer.setNativeArguments("/S /UPDATE=\""+QDir::toNativeSeparators(request)+"\"");
    installer.setWorkingDirectory(work);
    if (!installer.startDetached()) { error="Could not start the downloaded installer. Terlox is still running."; return false; }
#else
    const auto script=work+"/install.sh";
    const QByteArray body=R"SH(#!/bin/sh
pid=$1; kind=$2; package=$3; target=$4; restart=$5; report=$6; work=$7
exec >"$work/install.log" 2>&1
count=0
while kill -0 "$pid" 2>/dev/null; do
    count=$((count+1))
    if [ "$count" -gt 300 ]; then exit 1; fi
    sleep 0.2
done
ok=false
case "$kind" in
  appimage)
    stage=$(mktemp "$target.terlox-update.XXXXXX")
    if [ -n "$stage" ]; then
      if cp "$package" "$stage" && chmod 755 "$stage" && mv -f "$stage" "$target"; then ok=true; fi
      rm -f "$stage"
    fi
    ;;
  deb)
    if pkexec /usr/bin/apt-get install -y -- "$package"; then ok=true; fi
    ;;
  macos)
    if /usr/bin/osascript "$work/install.applescript" "$package"; then ok=true; fi
    ;;
esac
if [ "$ok" = true ]; then
    printf '%s' '{"success":true}' >"$report"
else
    printf '%s' '{"success":false,"message":"The update could not be installed. Your previous app will reopen. See install.log beside this report for details."}' >"$report"
fi
unset APPIMAGE APPDIR LD_LIBRARY_PATH LD_PRELOAD DYLD_LIBRARY_PATH DYLD_FRAMEWORK_PATH QT_PLUGIN_PATH QT_QPA_PLATFORM_PLUGIN_PATH QML2_IMPORT_PATH
"$restart" --update-result "$report" </dev/null >/dev/null 2>&1 &
)SH";
    if (!save(script,body,error)) return false;
    if (plan.kind=="macos" && !save(work+"/install.applescript",R"AS(on run argv
  do shell script "/usr/sbin/installer -pkg " & quoted form of item 1 of argv & " -target /" with administrator privileges
end run
)AS",error)) return false;
    QProcess worker; worker.setProgram("/bin/sh");
    worker.setArguments({script,QString::number(parentPid),plan.kind,package,plan.destination,plan.restart,report,work});
    worker.setWorkingDirectory(work);
    if (!worker.startDetached()) { error="Could not start the update installer. Terlox is still running."; return false; }
#endif
    return true;
}
}
