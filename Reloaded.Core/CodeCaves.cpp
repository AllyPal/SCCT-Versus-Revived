#include "pch.h"
#include "CodeCaves.h"
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
#include "Graphics.h"

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

int InstaFixEntry = 0x10AB8DAA;
__declspec(naked) void InstaFix() {
    static int InstaFixResume = 0x10AB8DE9;
    __asm {
        jmp dword ptr[InstaFixResume];
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

bool binocularInitialized = false;
const long binocularUpdateInterval = 33333333;
float baz1Initial;
float baz2Initial;
float baz3Initial;


std::chrono::steady_clock::time_point nextBinocularUpdate;
void binocularFix() {
    if (!binocularInitialized) {
        if (Graphics::lastFrameTime == std::chrono::steady_clock::time_point{}) {
            // last frame not set yet
            return;
        }
        baz1Initial = CodeCaves::lvIn->lPlC()->baz1();
        baz2Initial = CodeCaves::lvIn->lPlC()->baz2();
        baz3Initial = CodeCaves::lvIn->lPlC()->baz3();
        nextBinocularUpdate = Graphics::lastFrameTime;
        binocularInitialized = true;
    }
    if (nextBinocularUpdate <= Graphics::lastFrameTime) {
        CodeCaves::lvIn->lPlC()->baz1() = baz1Initial;
        CodeCaves::lvIn->lPlC()->baz2() = baz2Initial;
        CodeCaves::lvIn->lPlC()->baz3() = baz3Initial;
        auto remainder = std::chrono::duration_cast<std::chrono::nanoseconds>(Graphics::lastFrameTime - nextBinocularUpdate).count();
        nextBinocularUpdate = Graphics::lastFrameTime + std::chrono::nanoseconds(binocularUpdateInterval - remainder);

    }
    else {
        CodeCaves::lvIn->lPlC()->baz1() = 0.0f;
        CodeCaves::lvIn->lPlC()->baz2() = 0.0f;
        CodeCaves::lvIn->lPlC()->baz3() = 0.0f;
    }
}

void onPageUpdate(GUIPageWaitLaunch* page)
{
    static GUIPageWaitLaunch* lastPage;
    if (page != lastPage && page->Title() != nullptr && wcscmp(page->Title(), L"Game Options") == 0) {
        page->Components()->GetControl(3)->Components()->GetControl(2)->flags() &= ~8;
    }
    lastPage = page;
}

int GuiPageWaitUpdateEntry = 0x10C02C58;
__declspec(naked) void GuiPageWaitUpdate() {
    static GUIPageWaitLaunch* page;
    __asm {
        pushad
        mov [page], ecx
    }
    onPageUpdate(page);
    static int Return = 0x10A5A4C0;
    __asm {
        popad
        jmp dword ptr[Return]
    }
}

void onPageLoad(GUIPageWaitLaunch* page)
{
    static GUIPageWaitLaunch* lastPage;
    if (page != lastPage && page->Title() != nullptr && wcscmp(page->Title(), L"Video Options") == 0) {
        page->sRes() = reinterpret_cast<uintptr_t>(Graphics::videoSettingsDisplayModes);
        page->sResCount() = Graphics::GetResolutionCount();
        page->sResCount2() = Graphics::GetResolutionCount();
    }
    lastPage = page;
}

int GuiPageWaitLoadEntry = 0x10A66746;
__declspec(naked) void GuiPageWaitLoad() {
    static GUIPageWaitLaunch* page;
    __asm {
        je skip
        mov dword ptr[eax], 0x10C02C40
        mov[page], eax
        pushad
    }
    onPageLoad(page);
    __asm {
        popad
        skip:
        ret
    }
}

void forceSensInternal() {
    CodeCaves::lvIn->lPlC()->Sens() = 4.0f;
}

void CodeCaves::OncePerFrame() {
    binocularFix();
    forceSensInternal();
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
            mov     eax, 0x193
            mov[esp + 0x2c], eax
            popad
            mov     edx, [edi]
            mov     eax, 0x193
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

static int StickyCamContextMenuBlockEntry = 0x10B2D1C0;
__declspec(naked) void StickyCamContextMenuBlock() {
    __asm {
        mov eax, ecx
        mov eax, [eax + 0xB4]
        mov eax, [eax + 0x774]
        mov eax, [eax + 0xA90]
        mov [eax+0x37C], 0
        ret
    }
}


static std::map<std::pair<std::wstring, std::wstring>, std::wstring> overrideMap;
void CodeCaves::SetLabelOverride(const wchar_t* element, const wchar_t* page, std::wstring message) {
    overrideMap[{element, page}] = message;
}

static void InitLabelOverrides() {
    CodeCaves::SetLabelOverride(L"TitlePage", L"LAN_Menu", L"Reloaded Session");
    CodeCaves::SetLabelOverride(L"TitrePage.Caption", L"LAN_Menu", L"Reloaded Session");
    CodeCaves::SetLabelOverride(L"MainPage_Live.Caption", L"Menu_Multi", L" ");
    CodeCaves::SetLabelOverride(L"MainPage_LAN.Caption", L"Menu_Multi", L"Play Multiplayer");

    HKL layout = GetKeyboardLayout(0);
    UINT virtualKey = MapVirtualKeyEx(consoleKeyBind, MAPVK_VSC_TO_VK, layout);

    BYTE keyboardState[256] = { 0 };
    WCHAR buffer[3] = { 0 };
    int chars = ToUnicodeEx(virtualKey, consoleKeyBind, keyboardState, buffer, 2, 0, layout);

    if (chars > 0) {
        std::wstring message = L"Mouse Sensitivity - set with sens <value> in console (bind " + std::wstring(1, buffer[0]) + L")";
        CodeCaves::SetLabelOverride(L"MouseSensitive.Caption", L"Controller_Settings", message);
    }
    else {
        CodeCaves::SetLabelOverride(L"MouseSensitive.Caption", L"Controller_Settings", L"Mouse Sensitivity - set with sens <value> in console");
    }
}

void PrintTextEntry(wchar_t* _eax, wchar_t* _edi, wchar_t* _ebp, wchar_t* result) {
    if (result != nullptr) {
        std::wcout << std::format(L"eax:{} edi:{} ebp:{} result: {}", _eax, _edi, _ebp, result) << std::endl;
    }
}

wchar_t* OverrideLabel(wchar_t* languageName, wchar_t* controlName, wchar_t* menuName, wchar_t* current) {
#ifdef _DEBUG
    //PrintTextEntry(languageName, controlName, menuName, current);
#endif
    std::wstring controlKey(controlName);
    std::wstring menuKey(menuName);

    std::pair<std::wstring, std::wstring> key = { controlKey, menuKey };

    if (overrideMap.find(key) != overrideMap.end()) {
        return const_cast<wchar_t*>(overrideMap[key].c_str());
    }

    return current;
}

static int LoadTextEntry = 0x10912900;
__declspec(naked) void LoadText() {
    static int Return = 0x1091290C;
    static wchar_t* _eax;
    static wchar_t* _edi;
    static wchar_t* _ebp;
    static wchar_t* current;
    static wchar_t* textOverride;
    __asm {
        lea     eax, [esp + 0x8]
        push eax
        mov[_eax], eax
        push edi
        mov[_edi], edi
        push 0
        push ebp
        mov[_ebp], ebp
        call    dword ptr[edx + 0xC]
        mov[current],eax
        pushad
    } 
    textOverride = OverrideLabel(_eax, _edi, _ebp, current);
    __asm{
        popad
        mov eax,[textOverride]
        jmp dword ptr[Return]
    }
}

static void FragCreated(FragGrenade* frag) {
    if (frag->Timer() == 0.0f) {
        frag->Flags() &= ~0x1000;
    }
    else {
        frag->Flags() |= 0x1000;
    }
}

static int FragCreatedEntry = 0x10AF0443;
__declspec(naked) void FragCreated() {
    static FragGrenade* frag;
    __asm {
        pushad
        mov [frag], eax
    }
    FragCreated(frag);
    __asm {
        popad
        ret
    }
}

static int FragUpdatedEntry = 0x10BFAC14;
__declspec(naked) void FragUpdated() {
    static int Return = 0x10AB8990;
    static FragGrenade* frag;
    __asm {
        mov [frag], ecx
        pushad
    }
    FragCreated(frag);
    __asm {
        popad
        jmp dword ptr[Return]
    }
}

void CodeCaves::Initialize()
{
    InitLabelOverrides();

    MemoryWriter::WriteJump(InstaFixEntry, InstaFix);
    MemoryWriter::WriteJump(FragCreatedEntry, FragCreated);
    MemoryWriter::WriteFunctionPtr(FragUpdatedEntry, FragUpdated);

    MemoryWriter::WriteJump(SetLvInEntry, SetLvIn);
    MemoryWriter::WriteJump(OnStateChangeEntry, OnStateChange);
    MemoryWriter::WriteJump(LoadTextEntry, LoadText);

    MemoryWriter::WriteFunctionPtr(GuiPageWaitUpdateEntry, GuiPageWaitUpdate);
    MemoryWriter::WriteJump(GuiPageWaitLoadEntry, GuiPageWaitLoad);

    if (Config::disableStickyCamContextMenu) {
        MemoryWriter::WriteJump(StickyCamContextMenuBlockEntry, StickyCamContextMenuBlock);
        
        uint8_t shortJump[] = { 0xEB };
        MemoryWriter::WriteBytes(0x10B2CE1B, shortJump, sizeof(shortJump));
    }
}