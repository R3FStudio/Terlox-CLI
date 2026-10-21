#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdio>
namespace {
namespace fs=std::filesystem;
bool file(const fs::path& path) { std::error_code error; return fs::is_regular_file(path,error); }
bool completePackage(const fs::path& path) {
    return file(path/L".terlox-package.json") && file(path/L"terlox-setup-wizard.exe")
        && file(path/L"Qt6Core.dll") && file(path/L"Qt6Gui.dll") && file(path/L"Qt6Widgets.dll")
        && file(path/L"msvcp140.dll") && file(path/L"vcruntime140.dll") && file(path/L"vcruntime140_1.dll")
        && file(path/L"platforms/qwindows.dll");
}
std::wstring quote(const std::wstring& text) {
    std::wstring result=L"\""; size_t slashes=0;
    for (const auto c:text) {
        if (c==L'\\') { ++slashes; continue; }
        result.append(slashes*(c==L'"'?2:1),L'\\'); slashes=0;
        if (c==L'"') result+=L'\\'; result+=c;
    }
    result.append(slashes*2,L'\\'); return result+L'"';
}
int fail(const std::wstring& message,bool check) {
    if (check) fwprintf(stderr,L"%ls\n",message.c_str());
    else MessageBoxW(nullptr,message.c_str(),L"Terlox Setup",MB_OK|MB_ICONINFORMATION);
    return 2;
}
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    int count=0; auto** arguments=CommandLineToArgvW(GetCommandLineW(),&count);
    if (!arguments) return 2;
    std::vector<std::wstring> args; for (int i=1;i<count;++i) args.emplace_back(arguments[i]); LocalFree(arguments);
    // Keep path arguments relative to the caller, even though the wizard starts
    // with the deployed package as its working directory.
    for (size_t i=0;i<args.size();++i) {
        if ((args[i]==L"--package" || args[i]==L"--destination") && i+1<args.size()) {
            std::error_code error; const auto absolute=fs::absolute(args[i+1],error);
            if (!error) args[i+1]=absolute.wstring(); ++i;
        } else for (const auto* option:{L"--package=",L"--destination="}) {
            const std::wstring prefix(option);
            if (args[i].starts_with(prefix)) {
                std::error_code error; const auto absolute=fs::absolute(args[i].substr(prefix.size()),error);
                if (!error) args[i]=prefix+absolute.wstring();
            }
        }
    }
    bool check=false; fs::path requested;
    for (size_t i=0;i<args.size();++i) {
        if (args[i]==L"--check-package") check=true;
        if (args[i]==L"--package" && i+1<args.size()) requested=args[i+1];
        if (args[i].starts_with(L"--package=")) requested=args[i].substr(10);
    }
    std::wstring module(32768,L'\0'); const auto length=GetModuleFileNameW(nullptr,module.data(),DWORD(module.size()));
    if (!length || length>=module.size()) return fail(L"Could not locate the setup program.",check);
    module.resize(length); const auto directory=fs::path(module).parent_path();
    std::vector<fs::path> candidates;
    if (!requested.empty()) candidates.push_back(requested);
    candidates.push_back(directory);
    // A developer build contains no deployed Qt runtime or installable payload.
    // Its launcher opens the staged release instead of loading Qt from PATH.
    candidates.push_back(directory.parent_path()/L"package"/std::filesystem::path("windows-x64-" TERLOX_SETUP_VERSION));
    fs::path package;
    for (const auto& candidate:candidates) if (completePackage(candidate)) {
        std::error_code error; package=fs::absolute(candidate,error); if (!error) break; package.clear();
    }
    if (package.empty()) return fail(
        L"The complete Terlox installer folder could not be found.\n\n"
        L"Open the single Terlox setup EXE from dist instead of this developer launcher.\n\n"
        L"Building from source? Run scripts\\Package-Windows.ps1 first.",check);
    const auto executable=package/L"terlox-setup-wizard.exe";
    auto command=quote(executable.wstring()); for (const auto& arg:args) command+=L" "+quote(arg);
    STARTUPINFOW startup{sizeof(startup)}; PROCESS_INFORMATION process{};
    startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE); startup.hStdError=GetStdHandle(STD_ERROR_HANDLE);
    const auto valid=[](HANDLE handle) { return handle && handle!=INVALID_HANDLE_VALUE; };
    const bool redirected=valid(startup.hStdInput) && valid(startup.hStdOutput) && valid(startup.hStdError);
    if (redirected) startup.dwFlags=STARTF_USESTDHANDLES;
    if (!CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,redirected,0,nullptr,package.c_str(),&startup,&process))
        return fail(L"Terlox Setup could not start (Windows error "+std::to_wstring(GetLastError())+L").\n\nExtract the complete release into a writable folder and try again.",check);
    CloseHandle(process.hThread); WaitForSingleObject(process.hProcess,INFINITE);
    DWORD status=2; GetExitCodeProcess(process.hProcess,&status); CloseHandle(process.hProcess); return int(status);
}
