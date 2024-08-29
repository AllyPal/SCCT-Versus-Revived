#include <iostream>
#include <windows.h>
#include <stdio.h>
#include "Inject.h"

BOOL Inject::InjectDLL(HANDLE hProcess, const std::wstring& dllPath) {
    SIZE_T pathSize = (dllPath.size() + 1) * sizeof(wchar_t);

    LPVOID allocMem = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (allocMem == NULL) {
        return FALSE;
    }

    if (!WriteProcessMemory(hProcess, allocMem, dllPath.c_str(), pathSize, NULL)) {
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        return FALSE;
    }

    HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
    if (hKernel32 == 0) {
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        return FALSE;
    }
    LPVOID loadLibAddr = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryW");

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, allocMem, 0, NULL);
    if (hThread == NULL) {
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        return FALSE;
    }

    WaitForSingleObject(hThread, INFINITE);
    DWORD exitCode = 0;
    if (!GetExitCodeThread(hThread, &exitCode) || exitCode == 0) {
        return FALSE;
    }

    VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
    CloseHandle(hThread);

    return TRUE;
}
