#pragma once
#include <string>
#include <Windows.h>

struct SGaIn {
    std::byte unspecified[0x1000];
};

struct PlC {
    std::byte unspecified[0x1000];

    float& Defv() {
        return *reinterpret_cast<float*>(unspecified + (0x510));
    }

    float& Dfv() {
        return *reinterpret_cast<float*>(unspecified + (0xA44));
    }

    float& Sfv() {
        return *reinterpret_cast<float*>(unspecified + (0xA48));
    }
};

enum NetMode {
    NotMultiplayer,
    DedicatedServer,
    ListenServer,
    Client
};

struct LvIn {
    std::byte unspecified[0x1000];

    NetMode& netMode() {
        return *reinterpret_cast<NetMode*>(unspecified + (0x4A8));
    }

    SGaIn* sGaIn() {
        return *reinterpret_cast<SGaIn**>(unspecified + (0x4DC));
    }

    PlC* lPlC() {
        return *reinterpret_cast<PlC**>(unspecified + (0x514));
    }
};

struct GUIFont {
    std::byte unspecified[0x1000];

    wchar_t* KeyName() {
        return *reinterpret_cast<wchar_t**>(unspecified + (0x2C));
    }

    uint32_t* FirstFontArray() {
        return *reinterpret_cast<uint32_t**>(unspecified + (0x48));
    }
};

struct Console {
    std::byte unspecified[0x1000];
    uint32_t& ConsoleKey() {
        return *reinterpret_cast<uint32_t*>(unspecified + (0x34));
    }

    uint32_t& MyFont() {
        return *reinterpret_cast<uint32_t*>(unspecified + (0x1E8));
    }
};
