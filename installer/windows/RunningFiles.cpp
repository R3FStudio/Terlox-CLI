#include "InstalledApplication.h"
#include <QFileInfo>
#include <QDir>
#include <windows.h>
#include <restartmanager.h>
#include <vector>
namespace terlox::setup {
bool checkInstallationNotRunning(const UninstallPlan& plan,QString& error) {
    DWORD session=0; wchar_t key[CCH_RM_SESSION_KEY+1]{};
    if (RmStartSession(&session,0,key)!=ERROR_SUCCESS) { error="Windows could not check whether Terlox is running. Close its sessions and retry."; return false; }
    std::vector<std::wstring> paths; for (const auto& relative:plan.files) if (relative.endsWith(".exe",Qt::CaseInsensitive) || relative.endsWith(".dll",Qt::CaseInsensitive)) {
        const auto path=plan.directory+'/'+relative; if (QFileInfo::exists(path)) paths.push_back(QDir::toNativeSeparators(path).toStdWString());
    }
    std::vector<LPCWSTR> resources; for (const auto& path:paths) resources.push_back(path.c_str());
    if (resources.empty()) { RmEndSession(session); return true; }
    auto result=resources.empty()?ERROR_SUCCESS:RmRegisterResources(session,UINT(resources.size()),resources.data(),0,nullptr,0,nullptr);
    UINT required=0,count=0; DWORD reasons=0;
    if (result==ERROR_SUCCESS) result=RmGetList(session,&required,&count,nullptr,&reasons);
    QStringList names;
    if (result==ERROR_MORE_DATA) {
        std::vector<RM_PROCESS_INFO> processes(required); count=required;
        result=RmGetList(session,&required,&count,processes.data(),&reasons);
        if (result==ERROR_SUCCESS) for (UINT i=0;i<count;++i) names << QString::fromWCharArray(processes[i].strAppName);
    }
    RmEndSession(session);
    if (result!=ERROR_SUCCESS) { error="Windows could not check files in use. Close Terlox and retry."; return false; }
    if (!names.isEmpty()) { names.removeDuplicates(); error="Close these programs before uninstalling Terlox: "+names.join(", ")+". No programs were stopped."; return false; }
    return true;
}
}
