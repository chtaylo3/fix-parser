#include "PluginDefinition.h"

#include <windows.h>

extern NppData nppData;
extern FuncItem funcItem[];

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID /*lpReserved*/) {
    switch (reasonForCall) {
        case DLL_PROCESS_ATTACH:
            pluginInit(hModule);
            break;
        case DLL_PROCESS_DETACH:
            pluginCleanUp();
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData) {
    nppData = notpadPlusData;
    commandMenuInit();
}

extern "C" __declspec(dllexport) const wchar_t* getName() {
    return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* nbF) {
    *nbF = nbFunc;
    return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification* notifyCode) {
    handleNotification(notifyCode);
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT /*Message*/, WPARAM /*wParam*/,
                                                     LPARAM /*lParam*/) {
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL isUnicode() {
    return TRUE;
}
