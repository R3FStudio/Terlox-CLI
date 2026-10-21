#define PROXY_DELEGATION
#define PROXY_CLSID_IS {0x7e292492,0xda77,0x4af8,{0x93,0x5d,0x01,0xaf,0x34,0xb5,0x56,0xeb}}
#include <rpcproxy.h>
// A Terlox-owned proxy CLSID; the interface IDs remain Microsoft's ABI IDs.
EXTERN_PROXY_FILE(IConsoleHandoff)
EXTERN_PROXY_FILE(ITerminalHandoff)
PROXYFILE_LIST_START
REFERENCE_PROXY_FILE(IConsoleHandoff),
REFERENCE_PROXY_FILE(ITerminalHandoff),
PROXYFILE_LIST_END
DLLDATA_ROUTINES(aProxyFileList,GET_DLL_CLSID)
