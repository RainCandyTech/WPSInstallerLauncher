#include <Windows.h>
#include "KRSAVerifyFileHook.h"
#include "CreateFileHook.h"
#include "WPSProductUtil.h"

using namespace WPSProfileVerificationPatch;

BOOL APIENTRY DllMain(HMODULE module, DWORD reasonForCall, LPVOID reserved) {
    UNREFERENCED_PARAMETER(reserved);

    switch (reasonForCall) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(module);
            if (!WPSProductUtil::IsWPSOfficeProcess()) {
                MessageBoxW(nullptr, L"This process is not WPS Office. The patch will not be installed.", L"WPS Installer Launcher", MB_ICONINFORMATION);
                break;
            }
            KRSAVerifyFilePacketHook::Register();
            CreateFileHook::Register();
            HookManager::GetInstance().InstallHooks();
            break;
        case DLL_PROCESS_DETACH:
            HookManager::GetInstance().UninstallHooks();
            break;
    }
    return TRUE;
}
