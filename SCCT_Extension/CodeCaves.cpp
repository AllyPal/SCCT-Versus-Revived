#include "pch.h"
#include "CodeCaves.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

const double oneThousand = 1000.00f;
const float oneHundred = 100.00f;
const float one = 1.00f;
const double zero = 0.00f;

const int ToMilliseconds = 0x10B83AA0;
const int timeBeginPeriod = 0x10BDF53C;
const int timeEndPeriod = 0x10BDF538;
const int sleep = 0x10BDF108;

Logger* logger_;

static std::string WStringToString(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);

    return str;
}

static std::string toHexString(uintptr_t address) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << address;
    return ss.str();
}

uintptr_t scriptNamePtr;
void PrintUnrealScriptDebug() {
    wchar_t* unicodeStringPtr = reinterpret_cast<wchar_t*>(scriptNamePtr);
    std::wstring unicodeString(unicodeStringPtr);
    std::string output = WStringToString(unicodeString);
    if (output.compare(0, 3, "IK_") == 1) {//output.starts_with("IK_")) {
        std::cout << "Unreal Script: " << WStringToString(unicodeString) << std::endl;
    }
}

void PrintTest() {
    std::cout << "Redirected" << std::endl;
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

int result;
int sendMessage(SOCKET _socket, u_short hostshort, u_long hostlong, uintptr_t messagePtr, int messageLength) {
    struct sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;

    if (Config::useDirectConnect) {
#pragma warning(push)
#pragma warning(disable: 4996)
        to.sin_addr.s_addr = inet_addr(WStringToString(Config::directConnectIp).c_str());
#pragma warning(pop)
        to.sin_port = htons(hostshort);
    }
    else {
        to.sin_port = htons(hostshort);
        to.sin_addr.s_addr = htonl(hostlong);
    }

    char ipAddress[20];
    inet_ntop(AF_INET, &to.sin_addr, ipAddress, sizeof(ipAddress));

    std::cout << "Preparing to send message:" << std::endl;
    std::cout << "  sin_family: " << to.sin_family << std::endl;
    std::cout << "  sin_port: " << std::dec << ntohs(to.sin_port) << " (hostshort: " << hostshort << ")" << std::endl;
    std::cout << "  sin_addr.s_addr: " << ipAddress << " (hostlong: " << hostlong << ")" << std::endl;

    auto message = reinterpret_cast<char*>(messagePtr);
    int result = sendto(_socket, message, messageLength, 0, (struct sockaddr*)&to, sizeof(to));

    if (result == SOCKET_ERROR ) {
        std::cerr << "sendto failed with error: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "Message sent successfully. Bytes sent: " << result << std::endl;
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
int loc_0x1091FF70 = 0x1091FF85;

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

    PrintUnrealScriptDebug();
    __asm {
        popad
        jmp     dword ptr[loc_0x1091FF70]
    }
}

int frameRateLimit;
int fixSleepTimerEntry = 0x1095E340;
const int fixSleepTimerReturn = 0x1095E38D;
__declspec(naked) void fixSleepTimer() {
    __asm {
        mov     eax, dword ptr[frameRateLimit]
        fild    dword ptr[frameRateLimit]
        test    eax, eax
        jge     loc_1095E356
        mov     eax, 0x10C10A24
        fadd    dword ptr[eax]
        loc_1095E356:
        fdivr dword ptr[one]
        fld     st(1)
        fcomp   st(1)
        fnstsw  ax
        test    ah, 0x05
        jp      loc_1095E389
        fsub    st(0), st(1)
        fmul    qword ptr[oneThousand]
        call    dword ptr[ToMilliseconds]
        fstp    st(0)
        push    eax

        push    0x1
        mov     eax, dword ptr[timeBeginPeriod]
        call    dword ptr[eax]

        mov     eax, dword ptr[sleep]
        call    dword ptr[eax]

        push    0x1
        mov     eax, dword ptr[timeEndPeriod]
        call    dword ptr[eax]

        mov     edx, [esp + 00]
        jmp     fixSleepTimerEnd

        loc_1095E389 :
        fstp    st(0)
        fstp    st(0)

        fixSleepTimerEnd :
        jmp     dword ptr[fixSleepTimerReturn]
    }
}

int animatedTextureFixEntry = 0x109F2561;
__declspec(naked) void animatedTextureFix() {
    __asm {
        PUSH    ESI

        MOV     ESI, 0x10CCADA0
        FLD     QWORD PTR[ESI]

        FMUL    dword ptr[oneHundred]
        CALL    dword ptr[ToMilliseconds]
        POP     ESI

        test    eax, eax
        jz skipCheck

        // abusing this offset.  It's not used unless a FPS cap is specified on the animated texture
        cmp     dword ptr[esi + 0x0000008C], eax
        jg      skipTextureRender

        skipCheck:

        add     eax, 3 // 3 100ths of a second
        mov     dword ptr[esi + 0x0000008C], eax

        mov     edx, [esi]
        mov     ecx, esi
        call    dword ptr[edx + 0x000000A4]

        skipTextureRender:
        pop     esi
        add     esp, 0x08
        ret     0004
    }
}

int id = 0;
int flags = 0;
int unknown = 0;
uintptr_t dummy4;
int offs = 0;

void printTest() {
    wchar_t* unicodeStringPtr = reinterpret_cast<wchar_t*>(dummy4);
    std::wstring unicodeString(unicodeStringPtr);
    logger_->log(L"name: " + unicodeString);
    logger_->log("id: " + std::to_string(id));
    logger_->log("hexid: " + toHexString(id));
    logger_->log("hexidoffset: " + toHexString(id * 4));
    logger_->log("offs: " + toHexString(offs));
    logger_->log("offs*4: " + toHexString(offs * 4));
    logger_->log("flags: " + toHexString(flags));
    logger_->log("unknown: " + toHexString(unknown));

    
    logger_->log("");
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





bool __cdecl WriteJump(uintptr_t targetAddress, void(*function)()) {
    logger_->log("Writing jump at " + toHexString(targetAddress));
    uintptr_t functionAddress = reinterpret_cast<uintptr_t>(function);
    uintptr_t relativeAddress = (functionAddress - targetAddress - 5);
    uint8_t jump[5];
    jump[0] = 0xE9; // JMP opcode
    *reinterpret_cast<uint32_t*>(jump + 1) = static_cast<uint32_t>(relativeAddress);

    DWORD oldProtect;
    if (!VirtualProtect(reinterpret_cast<LPVOID>(targetAddress), sizeof(jump), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        logger_->log("Failed to change memory protection");
        return false;
    }

    memcpy(reinterpret_cast<void*>(targetAddress), jump, sizeof(jump));
    if (!VirtualProtect(reinterpret_cast<LPVOID>(targetAddress), sizeof(jump), oldProtect, &oldProtect)) {
        logger_->log("Failed to restore memory protection");
        return false;
    }

    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(targetAddress), sizeof(jump));
    logger_->log("Finished writing jump at " + toHexString(targetAddress));
    return true;
}

CodeCaves::CodeCaves(Logger* loggerIn) {
    logger_ = loggerIn;
}

#define DISSECT FALSE
void CodeCaves::Initialize()
{
    if (Config::applyAnimationFix)
        WriteJump(animatedTextureFixEntry, animatedTextureFix);
    frameRateLimit = Config::frameRateLimit;
    WriteJump(fixSleepTimerEntry, fixSleepTimer);

    WriteJump(sendBroadcastLanMessageEntry, sendBroadcastLanMessage);
    WriteJump(ServerInfoBroadcastEntry, ServerInfoBroadcast);
#if DISSECT
    WriteJump(unrealScriptNameDefinitionLookupEntry, unrealScriptNameDefinitionLookup);
    WriteJump(0x1093B590, test);
#endif
}