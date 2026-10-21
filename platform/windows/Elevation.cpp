#include "platform/Elevation.h"
#include "CommandLine.h"
#include <QFileInfo>
#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
namespace terlox {
bool isAdministrator() {
    HANDLE token=nullptr; if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token)) return false;
    TOKEN_ELEVATION elevation{}; DWORD size=0;
    const bool elevated=GetTokenInformation(token,TokenElevation,&elevation,sizeof(elevation),&size) && elevation.TokenIsElevated;
    CloseHandle(token); return elevated;
}
bool elevateTerminal(const ElevationRequest& request, QString& error, quintptr owner) {
    if (!QFileInfo::exists(request.executable)) { error="Cannot locate terlox.exe beside terloxsh.exe."; return false; }
    const auto parameters=windowsCommandLine("terlox",request.arguments).mid(7).toStdWString();
    const auto executable=request.executable.toStdWString(), directory=request.directory.toStdWString();
    SHELLEXECUTEINFOW launch{sizeof(launch)};
    launch.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC|SEE_MASK_FLAG_NO_UI;
    launch.hwnd=reinterpret_cast<HWND>(owner); launch.lpVerb=L"runas";
    launch.lpFile=executable.c_str(); launch.lpParameters=parameters.c_str(); launch.lpDirectory=directory.c_str(); launch.nShow=SW_SHOWNORMAL;
    const auto com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE);
    const bool started=ShellExecuteExW(&launch);
    const auto launchError=GetLastError();
    if (SUCCEEDED(com)) CoUninitialize();
    if (!started) {
        const auto code=launchError;
        error=code==ERROR_CANCELLED?"UAC request cancelled; no command was run.":QString("Windows elevation failed (error %1).").arg(code);
        return false;
    }
    if (launch.hProcess) CloseHandle(launch.hProcess);
    return true;
}
}
