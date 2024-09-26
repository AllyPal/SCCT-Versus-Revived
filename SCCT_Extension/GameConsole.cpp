#include "pch.h"
#include "GameConsole.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <dinput.h>
#include "include/d3d8/d3d8.h"
#include <format>
#include <set>
#include <iostream>
#include <thread>
#include <chrono>
#include <timeapi.h>
#include <Windows.h>
#include "StringOperations.h"
#include "MemoryWriter.h"
#include <functional>
#include <map>
#include <algorithm>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")

void PrintConsoleHelp();
void PrintConsoleValues();

static int thisConsole = 0;
int setThisConsoleEntry = 0x10B0F15E;
__declspec(naked) void setThisConsole() {
    static int Return = 0x10B0F165;
    __asm {
        lea     eax, dword ptr[esi + 0x28]
        mov[thisConsole], eax
        mov     dword ptr[esi + 0x28], 0x10BF280C
        jmp     dword ptr[Return]
    }
}

void GameConsole::WriteGameConsole(std::wstring message) {
    static int WriteConsoleFunc = 0x109117A0;
    if (!thisConsole) {
        return;
    }
    const wchar_t* messagePtr = message.c_str();
    __asm {
        pushad
        push messagePtr
        mov ebx, dword ptr[thisConsole]
        call dword ptr[WriteConsoleFunc]
        popad
    }
    return;
}

std::map<std::wstring, CommandHandler> getCommandHandlers() {
    std::map<std::wstring, CommandHandler> commandHandlers;

    if (Config::mouseInputFix) {
        commandHandlers[L"sens"] = {
            std::format(L"<number> - mouse sensitivity during gameplay.", Config::baseMouseSensitivity),
            [](const std::wstring& arg) {
            if (!arg.empty()) {
                Config::baseMouseSensitivity = std::stof(arg);
            Config::Serialize();
            }
            GameConsole::WriteGameConsole(std::format(L" > m_sens {:.3f}", Config::baseMouseSensitivity));
            },
            std::format(L" m_sens {:.3f}", Config::baseMouseSensitivity)
        };

        commandHandlers[L"sens_menu"] = {
            std::format(L"<number> - mouse sensitivity in menus.", Config::menuSensitivity),
            [](const std::wstring& arg) {
            if (!arg.empty()) {
                Config::menuSensitivity = std::stof(arg);
            Config::Serialize();
            }
            GameConsole::WriteGameConsole(std::format(L" > m_menu_sens {:.3f}", Config::menuSensitivity));
            },
            std::format(L" m_menu_sens {:.3f}", Config::menuSensitivity)
        };
    }

    commandHandlers[L"fps_client"] = {
        L"<number> - FPS whilst connected to servers.",
        [](const std::wstring& arg) {
        if (!arg.empty()) {
            auto frameLimit = std::stoi(arg);
            if (frameLimit < 30) {
                frameLimit = 30;
            }
            Config::frameRateLimit_client = frameLimit;
            Config::Serialize();
        }
        GameConsole::WriteGameConsole(std::format(L" > d_fps_client {}", Config::frameRateLimit_client));
        },
        std::format(L" d_fps_client {}", Config::frameRateLimit_client)
    };

    commandHandlers[L"fps_host"] = {
        L"<number> - FPS whilst hosting.",
        [](const std::wstring& arg) {
        if (!arg.empty()) {
            auto frameLimit = std::stoi(arg);
#ifndef _DEBUG
            if (frameLimit < 30) {
                frameLimit = 30;
                GameConsole::WriteGameConsole(std::format(L" > minimum setting is 30 FPS", Config::frameRateLimit_hosting));
            } else if (frameLimit > 90) {
                frameLimit = 90;
                GameConsole::WriteGameConsole(std::format(L" > maximum setting is currently 90 FPS whilst hosting.", Config::frameRateLimit_hosting));
            }
#endif
            Config::frameRateLimit_hosting = frameLimit;
            Config::Serialize();
        }
        GameConsole::WriteGameConsole(std::format(L" > d_fps_hosting {}", Config::frameRateLimit_hosting));
        },
        std::format(L" d_fps_hosting {}", Config::frameRateLimit_hosting)
    };

    commandHandlers[L"quit"] = {
        L"- Exits the game.",
        [](const std::wstring& arg) {
        exit(0);
        }
    };

    commandHandlers[L"test1"] = {
        L"- test1",
        [](const std::wstring& arg) {
            const uint8_t NOP = 0x90;
            uint8_t nops[] = { NOP, NOP };
            MemoryWriter::WriteBytes(0x10AA0535, nops, sizeof(nops));
        }
    };

    commandHandlers[L"help"] = {
        L"- Display command list.",
        [](const std::wstring& arg) {
            PrintConsoleHelp();
        }
    };

    commandHandlers[L"current"] = {
        L"- Display all current settings.",
        [](const std::wstring& arg) {
            PrintConsoleValues();
        }
    };
    return commandHandlers;
}

void PrintConsoleValues() {
    auto commandHandlers = getCommandHandlers();
    for (const auto& [key, handler] : commandHandlers) {
        if (!handler.displayValue.empty()) {
            GameConsole::WriteGameConsole(handler.displayValue);
        }
    }
}

void PrintConsoleHelp() {
    auto commandHandlers = getCommandHandlers();

    for (const auto& [key, handler] : commandHandlers) {
        GameConsole::WriteGameConsole(std::format(L" {} {}", key, handler.description));
    }
}

void ProcessConsole(uintptr_t inputPtr) {
    wchar_t* input = reinterpret_cast<wchar_t*>(inputPtr);
    std::wcout << L"Input: " << input << std::endl;

    std::wistringstream iss(input);
    std::wstring command, arg;
    iss >> command >> arg;
    std::transform(command.begin(), command.end(), command.begin(), ::towlower);
    auto commandHandlers = getCommandHandlers();
    if (commandHandlers.find(command) != commandHandlers.end()) {
        try {
            commandHandlers[command].handler(arg);
        }
        catch (...) {
            GameConsole::WriteGameConsole(L"Unexpected input format.");
        }
    }
}

int ConsoleInputEntry = 0x10B0F639;
__declspec(naked) void ConsoleInput() {
    static int Return = 0x10B0F63E;
    static uintptr_t inputPtr = 0;
    __asm {
        pushad
        mov[inputPtr], esi
    }
    ProcessConsole(inputPtr);
    __asm {
        popad
        push esi
        lea eax, [edi - 0x2C]
        push eax
        jmp dword ptr[Return]
    }
}

void GameConsole::Initialize()
{
    MemoryWriter::WriteJump(ConsoleInputEntry, ConsoleInput);
    MemoryWriter::WriteJump(setThisConsoleEntry, setThisConsole);
}
