// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h"
#include "HookHeader.h"

DWORD HookMain() {
    FILE* pFile = nullptr;
    if (AllocConsole()) {
        freopen_s(&pFile, "CONIN$", "rb", stdin);
        freopen_s(&pFile, "CONOUT$", "wb", stdout);
        freopen_s(&pFile, "CONOUT$", "wb", stderr);
    }
    else
    {
        return 0;
    }

    //dummy device hooking
    if (!d3dhelper::GetD3D11Device(dtable, sizeof(dtable)))
    {
        DLogs(0, "Create Dummy Device Fail");
        return 0;
    }
    DWORD64 pPresent = (DWORD64)dtable[8];

    cout << "pPresent : " << setbase(16) << pPresent << endl;
    
    /*if (oPresent) {
        oPresent = (tPresent)hook::hookTramp(pPresent, (DWORD64)hook::hEndScene, 18);
    }

    if (!oPresent)
    {
        DLogs(0, "EndScene Hook Fail");
    }*/

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HookMain, hModule, 0, nullptr));
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

