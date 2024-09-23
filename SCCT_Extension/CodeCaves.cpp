#include "pch.h"
#include "CodeCaves.h"
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
#include "Input.h"
#include "GameStructs.h"
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")

uintptr_t scriptNamePtr;
void PrintUnrealScriptDebug() {
    wchar_t* unicodeStringPtr = reinterpret_cast<wchar_t*>(scriptNamePtr);
    std::wstring unicodeString(unicodeStringPtr);
    std::string output = StringOperations::WStringToString(unicodeString);
    if (output.compare(0, 3, "IK_") == 1) {//output.starts_with("IK_")) {
        std::cout << "Unreal Script: " << StringOperations::WStringToString(unicodeString) << std::endl;
    }
}

void PrintTest() {
    std::cout << "Redirected" << std::endl;
}

LvIn* CodeCaves::lvIn = nullptr;
int SetLvInEntry = 0x109ADFB3;
__declspec(naked) void SetLvIn() {
    __asm {
        mov [CodeCaves::lvIn], esi
        ret
    }
}

bool CodeCaves::IsListenServer() {
    if (CodeCaves::lvIn == nullptr) {
        return false;
    }
    return CodeCaves::lvIn->netMode() == NetMode::ListenServer;
}



int ServerInfoBroadcastEntry = 0x10AB3E35;
__declspec(naked) void ServerInfoBroadcast() {
    //if (!CodeCaves::configRef.useDirectConnect) {
        /*__asm {
            mov[esp + 0x20], ecx
        }*/
    //}

    static int ServerInfoBroadcastReturn = 0x10AB3E3B;
    __asm {
        call edi
        jmp dword ptr[ServerInfoBroadcastReturn]
    }
}

int InstaFixPrototypeEntry = 0x10AB8DAA;
__declspec(naked) void InstaFixPrototype() {
    static int InstaFixPrototypeResume = 0x10AB8DE9;
    __asm {
        jmp dword ptr[InstaFixPrototypeResume];
    }
}



typedef LONG NTSTATUS;
typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(OSVERSIONINFOEXW*);

bool IsWindows10OrGreater() {
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        RtlGetVersionPtr pRtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (pRtlGetVersion) {
    OSVERSIONINFOEXW osInfo = { 0 };
    osInfo.dwOSVersionInfoSize = sizeof(osInfo);
            if (pRtlGetVersion(&osInfo) == 0) {
                std::cout << "win: " << osInfo.dwMajorVersion << std::endl;
        return osInfo.dwMajorVersion >= 10;
    }
        }
    }
    
    return false;
}

// Enables features which reduce the risks of potential buffer overflow vulernabilities in the base game
void CodeCaves::EnableProcessSecurity()
{
    if (Config::security_acg && IsWindows10OrGreater()) {
        PROCESS_MITIGATION_DYNAMIC_CODE_POLICY policy = {};
        policy.ProhibitDynamicCode = 1;
        if (SetProcessMitigationPolicy(ProcessDynamicCodePolicy, &policy, sizeof(policy))) {
            std::cout << "ACG enabled" << std::endl;
        }
    }

    if (Config::security_dep && SetProcessDEPPolicy(PROCESS_DEP_ENABLE)) {
        std::cout << "DEP enabled" << std::endl;
    }
}


//typedef wchar_t* (__cdecl* GetFriendlyErrorSig)(int Error);
//
//wchar_t* GetFriendlyError(int Error) {
//
//    void* functionAddress = reinterpret_cast<void*>(0x10A85BB0);
//    // Cast the address to the function pointer type
//    GetFriendlyErrorSig func = reinterpret_cast<GetFriendlyErrorSig>(functionAddress);
//
//    // Call the function using the function pointer
//    return func(Error);
//}

//_declspec(naked) void ToggleBroadcast() {
//    const char optVal = 1;
//    setsockopt(socket, SOL_SOCKET, SO_BROADCAST, &optVal, 4);
//}

int SendPacket(sockaddr_in& to, uintptr_t messagePtr, SOCKET _socket, int messageLength)
{
    char ipAddress[20];
    inet_ntop(AF_INET, &to.sin_addr, ipAddress, sizeof(ipAddress));

    std::cout << "Preparing to send message:" << std::endl;
    std::cout << "  sin_family: " << to.sin_family << std::endl;
    std::cout << "  sin_port: " << std::dec << ntohs(to.sin_port) << std::endl;
    std::cout << "  sin_addr.s_addr: " << ipAddress << std::endl;

    auto message = reinterpret_cast<char*>(messagePtr);
    int result = sendto(_socket, message, messageLength, 0, (struct sockaddr*)&to, sizeof(to));

    if (result == SOCKET_ERROR) {
        std::cerr << "sendto failed with error: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "Message sent successfully. Bytes sent: " << result << std::endl;
    }

    return result;
}
int result;
int sendMessage(SOCKET _socket, u_short hostshort, u_long hostlong, uintptr_t messagePtr, int messageLength) {
    struct sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    
    // Send broadcast packet
    to.sin_port = htons(hostshort);
    to.sin_addr.s_addr = htonl(hostlong);
    int result = SendPacket(to, messagePtr, _socket, messageLength);

    if (Config::useDirectConnect) {
#pragma warning(push)
#pragma warning(disable: 4996)
        to.sin_addr.s_addr = inet_addr(StringOperations::WStringToString(Config::directConnectIp).c_str());
#pragma warning(pop)
        to.sin_port = htons(hostshort);
        auto directConnectResult = SendPacket(to, messagePtr, _socket, messageLength);
        if (directConnectResult != SOCKET_ERROR) {
            result = directConnectResult;
        }
    }

    for (const auto& ipPortStr : Config::serverList) {
        size_t colonPos = ipPortStr.find(':');
        if (colonPos == std::string::npos) {
            std::cerr << "Invalid IP:PORT format: " << ipPortStr << std::endl;
            continue;
        }

        std::string ip = ipPortStr.substr(0, colonPos);
        std::string portStr = ipPortStr.substr(colonPos + 1);

        unsigned short port = static_cast<unsigned short>(std::stoi(portStr));

        sockaddr_in to = {};
        to.sin_family = AF_INET;

        if (inet_pton(AF_INET, ip.c_str(), &to.sin_addr) != 1) {
            std::cerr << "Invalid IP address format: " << ip << std::endl;
            continue;
        }

        to.sin_port = htons(port);

        int slResult = SendPacket(to, messagePtr, _socket, messageLength);
        if (slResult == SOCKET_ERROR) {
            std::cerr << "Failed to send packet to " << ipPortStr << std::endl;
        }
        else {
            result = slResult;
        }
    }

    return result;
}

static int sendBroadcastLanMessageEntry = 0x10AB3911;
__declspec(naked) void sendBroadcastLanMessage() {
    static SOCKET _socket;
    static u_short hostshort;
    static u_long hostlong;
    static uintptr_t messagePtr;
    __asm {
        lea     edx, [esp + 0x24]
        mov     dword ptr[messagePtr], edx
        mov     dx, [edi + 0x300]
        mov     word ptr[hostshort], dx
        mov     edx, dword ptr[edi + 0x2FC]
        mov     dword ptr[_socket], edx
        mov     edx, [edi + 0x314]
        mov     dword ptr[hostlong], edx
    }

    result = sendMessage(_socket, hostshort, hostlong, messagePtr, 40);

    static int sendBroadcastLanMessageReturn = 0x10AB3953;
    __asm {
        mov     eax, dword ptr[result]
        jmp     dword ptr[sendBroadcastLanMessageReturn]
    }
}

int unrealScriptNameDefinitionLookupEntry = 0x1091FF70;
__declspec(naked) void unrealScriptNameDefinitionLookup() {
    __asm {
        push    edi
        mov     edi, eax
        cmp     word ptr[edi], 00
        jne     nameDefined
        mov     dword ptr[ebx], 00000000
        mov     eax, ebx
        pop     edi
        ret     0004

        nameDefined:
        mov     dword ptr[scriptNamePtr], eax
            pushad
    }

    static int loc_0x1091FF70 = 0x1091FF85;
    PrintUnrealScriptDebug();
    __asm {
        popad
        jmp     dword ptr[loc_0x1091FF70]
    }
}

float GetPerformance() {
    static int getPerformanceAddress = 0x10904030;
    float result;
    __asm {
        call dword ptr[getPerformanceAddress]
        fstp dword ptr[result]
    }
    return result;
}



bool ValidateState(int newState) {
    bool result = true;
    __asm {
        mov eax, newState
        mov ecx, 0xF4522FF2
        xor ecx, -1
        mov ebx, 0
        mov eax, 0x1B3D94FC
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B39F643
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B39F653
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B39FCA0
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B39E4EB
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B39982A
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B398314
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B0D7F82
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B0C6D75
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B0F27D3
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B0429E8
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B042A7C
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B07D0E5
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B07D119
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B07D444
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B07C282
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B064916
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B061F52
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B060F84
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B002FE5
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B02B02B
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B1FA6AA
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B1FAF1E
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B1F1F73
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B1ED0C1
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B1EC606
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B1EC74B
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B179997
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B103B89
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B39E89B
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B3999E2
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B3984E4
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B3A45FB
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B3A4603
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B0C9F1B
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B07D0FF
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B07D520
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        mov eax, 0x1B07CE18
        xor eax, ecx
        mov eax, [eax]
        add ebx, eax
        ror ebx, 1
        cmp ebx, 0x167c46f3
        je skip
        mov byte ptr[result], 0
        skip:
    }
    return result;
}

int OnStateChangeEntry = 0x109F1FC3;
__declspec(naked) void OnStateChange() {
    static int Deny = 0x109F1FD0;
    static int Return = 0x109F1FCB;
    static int currentState;
    static int newState;
    static int all;
    __asm {
        pushad
        mov dword ptr[currentState], esi
        mov dword ptr[newState], eax
    }
    
    if (!ValidateState(newState)) {
        __asm {
            mov     eax, 0x13BFF3
            mov[esp + 0x2c], eax
            popad
            mov     edx, [edi]
            mov     eax, 0x13BFF3
            push    eax
            mov     ecx, edi
            call    dword ptr[edx + 0x30]
            jmp     dword ptr[Return]
        }
    }
    else {
        __asm {
            popad
            mov     edx, [edi]
            push    eax
            mov     ecx, edi
            call    dword ptr[edx + 0x30]
            jmp     dword ptr[Return]
        }
    }
}

//static int StickyCamContextMenuBlock1Entry = 0x10B2D2D3;
//__declspec(naked) void StickyCamContextMenuBlock1() {
//    static int Return = 0x10B2D2D9;
//    __asm {
//        xor edx, edx
//        mov[ebx + 0x37C], edx
//        jmp dword ptr[Return]
//    }
//}
//
//static int StickyCamContextMenuBlock2Entry = 0x10B2D263;
//__declspec(naked) void StickyCamContextMenuBlock2() {
//    static int Return = 0x10B2D269;
//    __asm {
//        mov[ebx + 0x37C], 0
//        cmp esi, [ebx + 0x37C]
//        jmp dword ptr[Return]
//    }
//}

static int StickyCamContextMenuBlock3Entry = 0x10B2D1C0;
__declspec(naked) void StickyCamContextMenuBlock3() {
    __asm {
        mov eax, ecx
        mov eax, [eax + 0xB4]
        mov eax, [eax + 0x774]
        mov eax, [eax + 0xA90]
        mov [eax+0x37C], 0
        ret
    }
}

int id = 0;
int flags = 0;
int unknown = 0;
uintptr_t dummy4;
int offs = 0;
int addr = 0;

void printTest() {
    wchar_t* unicodeStringPtr = reinterpret_cast<wchar_t*>(dummy4);
    std::wstring unicodeString(unicodeStringPtr);
    Logger::log(L"name: " + unicodeString);
    Logger::log("id: " + std::to_string(id));
    Logger::log("hexid: " + StringOperations::toHexString(id));
    Logger::log("hexidoffset: " + StringOperations::toHexString(id * 4));
    Logger::log("flags: " + StringOperations::toHexString(flags));
    Logger::log("unknown: " + StringOperations::toHexString(unknown));
    Logger::log("addr: " + StringOperations::toHexString(addr));

    Logger::log("");
}

int imp_wcslen = 0x10BDF3B4;
int imp_wcscpy = 0x10BDF39C;

int get_uc_func_offset = 0x1090F6B0;

//1093B590
__declspec(naked) void test() {
    __asm {
        mov eax, 0x10C73BF0
        mov eax, dword ptr[eax]
        push esi
        mov esi, [eax]
        push 0x10C6F850
        push edi

        mov eax, dword ptr[imp_wcslen]
        call[eax]

        lea ecx, [eax + eax + 0x0E]
        add esp, 0x04
        push ecx
        mov ecx, 0x10C73BF0
        mov ecx, dword ptr[ecx]
        call dword ptr[esi]
        mov edx, [esp + 0x08]
        mov ecx, [esp + 0x10]
        mov esi, eax
        mov eax, [esp + 0x0C]
        mov[esi], edx
        lea edx, [esi + 0x0C]
        push edi
        push edx
        mov[esi + 0x04], eax
        mov[esi + 0x08], ecx

        mov eax, dword ptr[imp_wcscpy]
        call[eax]
        mov [addr], esi
        mov eax, [esi]
        mov [id], eax
        mov eax, [esi+4]
        mov[flags], eax
        mov eax, [esi+8]
        mov[unknown], eax
        mov eax, esi
        add eax, 0xc
        mov [dummy4], eax

        pushad

        LEA eax, get_uc_func_offset
        call [get_uc_func_offset]
        and eax, 0x0FFF
        mov[offs], eax
        /*mov eax, [esi]
        cmp eax, 0x13C2CD
        jne skip1

        int 3
        skip1:*/
    }
    printTest();
    //here
    __asm{
        popad
        add esp, 0x08
        mov eax, esi
        pop esi
        ret
    }
}

#define DISSECT FALSE
void CodeCaves::Initialize()
{
    MemoryWriter::WriteJump(sendBroadcastLanMessageEntry, sendBroadcastLanMessage);
    MemoryWriter::WriteJump(ServerInfoBroadcastEntry, ServerInfoBroadcast);
    MemoryWriter::WriteJump(InstaFixPrototypeEntry, InstaFixPrototype);
    MemoryWriter::WriteJump(SetLvInEntry, SetLvIn);
    MemoryWriter::WriteJump(OnStateChangeEntry, OnStateChange);

    if (Config::disableStickyCamContextMenu) {
        MemoryWriter::WriteJump(StickyCamContextMenuBlock3Entry, StickyCamContextMenuBlock3);
        
        uint8_t shortJump[] = { 0xEB };
        MemoryWriter::WriteBytes(0x10B2CE1B, shortJump, sizeof(shortJump));
    }
}