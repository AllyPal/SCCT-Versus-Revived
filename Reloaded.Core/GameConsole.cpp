#include "pch.h"
#include "GameConsole.h"
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
#include "GameStructs.h"
#include "Fonts.h"
#include "Debug.h"
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")

void PrintConsoleHelp();
void PrintConsoleValues();

int getGraveAccentKeyCode() {
    HKL layout = GetKeyboardLayout(0);
    UINT vKey = MapVirtualKeyEx(consoleKeyBind, MAPVK_VSC_TO_VK, layout);
    return vKey & 0xFF;
}

static Console* console;

void OnConsoleCreated() {
    console->ConsoleKey() = getGraveAccentKeyCode();
}

static bool initialized = false;
void OnToggleConsole() {
    if (initialized) return;
    GUIFont* font = Fonts::GetFontByKeyName(L"GUIVerySmallDenFont");
    if (font->FirstFontArray() != nullptr) {
        console->MyFont() = *font->FirstFontArray();

        GameConsole::WriteGameConsole(L"======================");
        GameConsole::WriteGameConsole(L"SCCT Versus Reloaded vI");
        GameConsole::WriteGameConsole(L"======================");
        GameConsole::WriteGameConsole(L" ");
        GameConsole::WriteGameConsole(L"Type 'help' to view the command list");

        initialized = true;
    }
}

static int ConsoleToggledEntry = 0x10C07768;
__declspec(naked) void ConsoleToggled() {
    static int Return = 0x1094A300;
    __asm {
        pushad
    }
    OnToggleConsole();
    __asm {
        popad
        jmp dword ptr[Return]
    }
}

static int ConsoleCreatedEntry = 0x109AB58B;
__declspec(naked) void ConsoleCreated() {
    __asm {
        pushad
        mov [console], esi
    }
    OnConsoleCreated();
    __asm {
        popad
        add     esp, 0x10
        ret
    }
}

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

    commandHandlers[L"widescreen"] = {
            std::format(L"<true/false> - apply widescreen aspect ratio.", Config::baseMouseSensitivity),
            [](const std::wstring& arg) {
            if (!arg.empty()) {
                std::wstring lArg = StringOperations::toLowercase(arg);
                if (lArg == L"true") {
                    Config::widescreenAspectRatioFix = true;
                }
                else if (lArg == L"false") {
                    Config::widescreenAspectRatioFix = false;
                }
                else {
                    throw std::runtime_error("Unexpected argument");
                }
                Config::Serialize();
            }
            GameConsole::WriteGameConsole(std::format(L" > widescreen {}", Config::widescreenAspectRatioFix ? L"true" : L"false"));
            },
            std::format(L" widescreen {}", Config::widescreenAspectRatioFix ? L"true" : L"false")
    };

    if (Config::mouseInputFix) {
        commandHandlers[L"sens"] = {
            std::format(L"<number> - mouse sensitivity during gameplay.", Config::baseMouseSensitivity),
            [](const std::wstring& arg) {
            if (!arg.empty()) {
                Config::baseMouseSensitivity = std::stof(arg);
                Config::Serialize();
            }
            GameConsole::WriteGameConsole(std::format(L" > sens {:.3f}", Config::baseMouseSensitivity));
            },
            std::format(L" sens {:.3f}", Config::baseMouseSensitivity)
        };

        commandHandlers[L"sens_menu"] = {
            std::format(L"<number> - mouse sensitivity in menus.", Config::menuSensitivity),
            [](const std::wstring& arg) {
            if (!arg.empty()) {
                Config::menuSensitivity = std::stof(arg);
            Config::Serialize();
            }
            GameConsole::WriteGameConsole(std::format(L" > sens_menu {:.3f}", Config::menuSensitivity));
            },
            std::format(L" sens_menu {:.3f}", Config::menuSensitivity)
        };
    }

    commandHandlers[L"fps_client"] = {
        L"<number> - FPS whilst connected to servers.",
        [](const std::wstring& arg) {
        if (!arg.empty()) {
            auto frameLimit = std::stoi(arg);
#ifndef _DEBUG
            if (frameLimit < frameRateLimit_client_min) {
                frameLimit = frameRateLimit_client_min;
                GameConsole::WriteGameConsole(std::format(L" > minimum setting is {} FPS", frameRateLimit_client_min));
            }
            else if (frameLimit > frameRateLimit_client_max) {
                frameLimit = frameRateLimit_client_max;
                GameConsole::WriteGameConsole(std::format(L" > maximum setting is {} FPS.", frameRateLimit_client_max));
            }
#endif
            Config::frameRateLimit_client = frameLimit;
            Config::Serialize();
        }
        GameConsole::WriteGameConsole(std::format(L" > fps_client {}", Config::frameRateLimit_client));
        },
        std::format(L" fps_client {}", Config::frameRateLimit_client)
    };

    commandHandlers[L"fps_host"] = {
        L"<number> - FPS whilst hosting.",
        [](const std::wstring& arg) {
        if (!arg.empty()) {
            auto frameLimit = std::stoi(arg);
#ifndef _DEBUG
            if (frameLimit < frameRateLimit_hosting_min) {
                frameLimit = frameRateLimit_hosting_min;
                GameConsole::WriteGameConsole(std::format(L" > minimum setting is {} FPS", frameRateLimit_hosting_min));
            } else if (frameLimit > frameRateLimit_hosting_max) {
                frameLimit = frameRateLimit_hosting_max;
                GameConsole::WriteGameConsole(std::format(L" > maximum setting is currently {} FPS whilst hosting.", frameRateLimit_hosting_max));
            }
#endif
            Config::frameRateLimit_hosting = frameLimit;
            Config::Serialize();
        }
        GameConsole::WriteGameConsole(std::format(L" > fps_host {}", Config::frameRateLimit_hosting));
        },
        std::format(L" fps_host {}", Config::frameRateLimit_hosting)
    };

    commandHandlers[L"quit"] = {
        L"- Exits the game.",
        [](const std::wstring& arg) {
        exit(0);
        }
    };

    //commandHandlers[L"test1"] = {
    //    L"- test1",
    //    [](const std::wstring& arg) {
    //        const uint8_t NOP = 0x90;
    //        uint8_t nops[] = { NOP, NOP };
    //        MemoryWriter::WriteBytes(0x10AA0535, nops, sizeof(nops));
    //    }
    //};

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

//    commandHandlers[L"lod"] = {
//            std::format(L"<number> - higher numbers increase model detail at distance.", Config::lod),
//            [](const std::wstring& arg) {
//            if (!arg.empty()) {
//                auto lod = std::stof(arg);
//#ifndef _DEBUG
//                if (lod < 1.0) {
//                    lod = 1.0;
//                    GameConsole::WriteGameConsole(std::format(L" > minimum setting 1.0 (SCCT Versus Default)", Config::frameRateLimit_hosting));
//                }
//#endif
//                Config::lod = lod;
//                Config::Serialize();
//            }
//            GameConsole::WriteGameConsole(std::format(L" > lod {:.3f}", Config::lod));
//            },
//            std::format(L" lod {:.3f}", Config::lod)
//    };

    Debug::CommandHandlers(&commandHandlers);

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
    MemoryWriter::WriteJump(ConsoleCreatedEntry, ConsoleCreated);
    MemoryWriter::WriteFunctionPtr(ConsoleToggledEntry, ConsoleToggled);
}
