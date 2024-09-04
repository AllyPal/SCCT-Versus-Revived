// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <Windows.h>
#include <thread>
#include <chrono>
#include "include/nlohmann/json.hpp"
#include "logger.h"
#include "CodeCaves.h"
#include "Config.h"

INIT_ONCE g_InitOnce = INIT_ONCE_STATIC_INIT;

std::wstring GetDllPath(HINSTANCE hModule) {
    std::vector<wchar_t> pathBuffer(MAX_PATH);
    DWORD result = GetModuleFileName(hModule, pathBuffer.data(), static_cast<DWORD>(pathBuffer.size()));
    if (result == 0) {
        return L"";
    }

    pathBuffer.resize(result);
    return std::wstring(pathBuffer.begin(), pathBuffer.end());
}

static std::wstring GetExecutableDirectory(std::wstring executablePath) {
    std::wstring::size_type pos = std::wstring(executablePath).find_last_of(L"\\/");
    return std::wstring(executablePath).substr(0, pos);
}

void RedirectToConsole()
{
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stderr);

    std::cout << "SCCT Versus Revived injected successfully" << std::endl;
}



const int BaseAddress = 0x10900000;
Logger* logger = nullptr;
BOOL CALLBACK InitFunction(PINIT_ONCE InitOnce, PVOID Parameter, PVOID* Context) {
    HMODULE hModule = static_cast<HMODULE>(Parameter);
    auto dllPath = GetDllPath(hModule);
    auto directoryPath = GetExecutableDirectory(dllPath);
#ifdef _DEBUG
    RedirectToConsole();
#endif

    logger = new Logger(dllPath);
    auto configPath = directoryPath + L"\\SCCT_config.json";
    logger->log("");
    logger->log(L"Loading " + configPath);
    Config::Initialize(configPath);
    logger->log(L"applyAnimationFix: " + std::to_wstring(Config::applyAnimationFix));
    logger->log(L"frameRateLimit:" + std::to_wstring(Config::frameRateLimit));

    CodeCaves caves(logger);
    caves.Initialize();
    return TRUE;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            InitOnceExecuteOnce(&g_InitOnce, InitFunction, hModule, NULL);
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

