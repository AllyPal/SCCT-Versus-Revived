#include "pch.h"
#include "Networking.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <format>
#include <set>
#include <timeapi.h>
#include <Windows.h>
#include "StringOperations.h"
#include "MemoryWriter.h"
#include "Input.h"
#include "GameStructs.h"
#include "Config.h"

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

void Networking::Initialize()
{
    MemoryWriter::WriteJump(sendBroadcastLanMessageEntry, sendBroadcastLanMessage);
    MemoryWriter::WriteJump(ServerInfoBroadcastEntry, ServerInfoBroadcast);
}