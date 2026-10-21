#pragma once
#include <windows.h>
namespace terlox {
struct ConptyApi {
    HMODULE module=nullptr;
    HRESULT (WINAPI *create)(COORD,HANDLE,HANDLE,DWORD,HPCON*)=nullptr;
    HRESULT (WINAPI *pack)(HANDLE,HANDLE,HANDLE,HPCON*)=nullptr;
    HRESULT (WINAPI *release)(HPCON)=nullptr;
    HRESULT (WINAPI *resize)(HPCON,COORD)=nullptr;
    void (WINAPI *close)(HPCON)=nullptr;
    bool available() const { return pack && release && resize && close; }
    bool canCreate() const { return create && resize && close; }
    static const ConptyApi& instance();
};
}
