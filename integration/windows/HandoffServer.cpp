#include "HandoffServer.h"
#include "HandoffIds.h"
#include "ConptyApi.h"
#include "ITerminalHandoff.h"
#include <QCoreApplication>
#include <QMetaObject>
#include <atomic>
#include <wrl.h>
namespace terlox {
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;
using Microsoft::WRL::ClassicCom;
using Callback=std::function<void(std::shared_ptr<InboundSession>)>;
static bool clone(HANDLE source,void*& result) { return DuplicateHandle(GetCurrentProcess(),source,GetCurrentProcess(),&result,0,FALSE,DUPLICATE_SAME_ACCESS); }
class Receiver final:public RuntimeClass<RuntimeClassFlags<ClassicCom>,ITerminalHandoff3,ITerminalHandoff2> {
public:
    Callback callback;
    std::atomic<bool> received{false};
    HRESULT deliver(HANDLE input,HANDLE output,HANDLE signal,HANDLE reference,HANDLE server,HANDLE client,const TERMINAL_STARTUP_INFO* startup) {
        if (received.exchange(true)) return HRESULT_FROM_WIN32(ERROR_BUSY);
        auto session=std::make_shared<InboundSession>();
        if (!clone(input,session->input)||!clone(output,session->output)||!clone(signal,session->signal)||!clone(reference,session->reference)||!clone(server,session->server)||!clone(client,session->client)) return HRESULT_FROM_WIN32(GetLastError());
        if (startup) session->title=QString::fromWCharArray(startup->pszTitle,SysStringLen(startup->pszTitle));
        QMetaObject::invokeMethod(QCoreApplication::instance(),[handler=callback,session] { handler(session); },Qt::QueuedConnection);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE EstablishPtyHandoff(HANDLE input,HANDLE output,HANDLE signal,HANDLE reference,HANDLE server,HANDLE client,TERMINAL_STARTUP_INFO startup) override { return deliver(input,output,signal,reference,server,client,&startup); }
    HRESULT STDMETHODCALLTYPE EstablishPtyHandoff(HANDLE* input,HANDLE* output,HANDLE signal,HANDLE reference,HANDLE server,HANDLE client,const TERMINAL_STARTUP_INFO* startup) override {
        if (!input||!output||!startup) return E_POINTER;
        *input=nullptr; *output=nullptr;
        HANDLE hostRead=nullptr,terminalWrite=nullptr,terminalRead=nullptr,hostWrite=nullptr;
        if (!CreatePipe(&hostRead,&terminalWrite,nullptr,0)) return HRESULT_FROM_WIN32(GetLastError());
        if (!CreatePipe(&terminalRead,&hostWrite,nullptr,0)) { const auto error=GetLastError(); CloseHandle(hostRead); CloseHandle(terminalWrite); return HRESULT_FROM_WIN32(error); }
        const auto result=deliver(terminalWrite,terminalRead,signal,reference,server,client,startup);
        CloseHandle(terminalWrite); CloseHandle(terminalRead);
        if (FAILED(result)) { CloseHandle(hostRead); CloseHandle(hostWrite); return result; }
        *input=hostRead; *output=hostWrite; return S_OK;
    }
};
class Factory final:public RuntimeClass<RuntimeClassFlags<ClassicCom>,IClassFactory> {
public:
    ComPtr<Receiver> receiver;
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer,REFIID iid,void** result) override { if (outer) return CLASS_E_NOAGGREGATION; return receiver->QueryInterface(iid,result); }
    HRESULT STDMETHODCALLTYPE LockServer(BOOL) override { return S_OK; }
};
struct HandoffServer::Impl { Callback callback; DWORD cookie=0; HRESULT initialized=E_FAIL; ComPtr<Factory> factory; };
HandoffServer::HandoffServer(Callback callback):d(std::make_unique<Impl>()) { d->callback=std::move(callback); }
HandoffServer::~HandoffServer() { if (d->cookie) CoRevokeClassObject(d->cookie); d->factory.Reset(); if (SUCCEEDED(d->initialized)) CoUninitialize(); }
bool HandoffServer::listen(QString& error) {
    if (!ConptyApi::instance().available()) { error="The packaged conpty.dll is missing or incompatible."; return false; }
    d->initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    if (FAILED(d->initialized) && d->initialized!=RPC_E_CHANGED_MODE) { error="COM initialization failed."; return false; }
    d->factory=Microsoft::WRL::Make<Factory>(); d->factory->receiver=Microsoft::WRL::Make<Receiver>(); d->factory->receiver->callback=d->callback;
    CLSID id; CLSIDFromString(QString(TerminalClass).toStdWString().c_str(),&id);
    const auto hr=CoRegisterClassObject(id,d->factory.Get(),CLSCTX_LOCAL_SERVER,REGCLS_SINGLEUSE,&d->cookie);
    if (FAILED(hr)) { error=QString("Could not listen for console handoff (0x%1).").arg(quint32(hr),0,16); return false; }
    return true;
}
}
