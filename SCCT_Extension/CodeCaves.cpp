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
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")

const double oneThousand = 1000.00f;
const float oneHundred = 100.00f;
const float one = 1.00f;
const double zero = 0.00f;

const int ToMilliseconds = 0x10B83AA0;
const int timeBeginPeriodAddr = 0x10BDF53C;
const int timeEndPeriodAddr = 0x10BDF538;
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

LvIn* lvIn = nullptr;
int SetLvInEntry = 0x109ADFB3;
__declspec(naked) void SetLvIn() {
    __asm {
        mov [lvIn], esi
        ret
    }
}

bool IsListenServer() {
    if (lvIn == nullptr) {
        return false;
    }
    return lvIn->netMode() == NetMode::ListenServer;
}

static int xMouseDelta = 0;
static int yMouseDelta = 0;

int DisableMouseInputEntry = 0x10B10CF3;
__declspec(naked) void DisableMouseInput() {
    static int DoNotProcess = 0x10B10CAE;
    static int KeepProcessing = 0x10B10CF8;
    __asm {
        cmp     eax, 0x10
        ja      doNotProcess
        cmp     eax, 0x0
        je      mouseX
        cmp     eax, 0x4
        je      mouseY

        doProcess:
        jmp     dword ptr[KeepProcessing]

        doNotProcess:
        jmp     dword ptr[DoNotProcess]

        mouseX:
        push eax
        mov eax, dword ptr[xMouseDelta]
        cmp eax, 0
        pop eax
        je doNotProcess
        jmp doProcess

        mouseY :
        push eax
        mov eax, dword ptr[yMouseDelta]
        cmp eax, 0
        pop eax
        je doNotProcess
        jmp doProcess
    }
}

float aspectRatioMenuVertMouseInputMultiplier = 1.0;
static float menuPositionX = 240.0f;
static float menuPositionY = 320.0f;

void HandleMouseInput(LPDIRECTINPUTDEVICE8 device, int dd) {
    DIMOUSESTATE2 mouseState;
    HRESULT hr = device->GetDeviceState(sizeof(DIMOUSESTATE2), &mouseState);

    if (FAILED(hr)) {
        if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
            std::cerr << "Device lost or not acquired. Attempting to reacquire..." << std::endl;
            hr = device->Acquire();
            if (FAILED(hr)) {
                std::cerr << "Failed to acquire device." << std::endl;
                return;
            }
            if (SUCCEEDED(hr)) {
                std::cerr << "Acquired device." << std::endl;
            }
            hr = device->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState);
        }

        if (FAILED(hr)) {
            std::cerr << "Failed to get device state. Error code: " << toHexString(hr) << std::endl;
            return;
        }
    }
    xMouseDelta = mouseState.lX;
    yMouseDelta = mouseState.lY;

    menuPositionY += yMouseDelta * Config::menuSensitivity * aspectRatioMenuVertMouseInputMultiplier;
    menuPositionY = std::clamp(menuPositionY, 0.0f, 480.0f);

    menuPositionX += xMouseDelta * Config::menuSensitivity;
    menuPositionX = std::clamp(menuPositionX, 0.0f, 640.0f);
}


int FixMouseInputEntry = 0x10B10C80;
__declspec(naked) void FixMouseInput() {
    static int DevicePtr = 0x10C91D04;
    static int dd;
    static LPDIRECTINPUTDEVICE8 device;
    __asm {
        pushad
        mov eax, DevicePtr
        cmp eax, 0
        je exitf
        mov eax, dword ptr[eax]
        je exitf
        mov dword ptr[device], eax
        mov dword ptr[dd], esi
    }
    HandleMouseInput(device, dd);
    static int Return = 0x10B10C86;
    __asm {
        exitf:
        popad
        test ebp, ebp
        mov[esp + 0x10], ebx
        jmp dword ptr[Return]
    }
}

int X_WriteMouseInputEntry = 0x10B10D06;
__declspec(naked) void X_WriteMouseInput() {
    static int Resume = 0x10B10D0D;

    __asm {
        mov eax, [esi + 0x18]
        fild dword ptr[xMouseDelta]
        jmp dword ptr[Resume]      
    }
}

int Y_WriteMouseInputEntry = 0x10B10D23;
__declspec(naked) void Y_WriteMouseInput() {
    static int Resume = 0x10B10D2D;


    __asm {
        mov     eax, [esi + 0x18]
        mov     ecx, [eax + 0x28]
        mov     eax, dword ptr[yMouseDelta]
        jmp dword ptr[Resume]
    }
}

const float ten = 10;
int BaseMouseSensitivityEntry = 0x109FC177;
__declspec(naked) void BaseMouseSensitivity() {
    __asm {
        fmul dword ptr[Config::baseMouseSensitivity]
        fdiv dword ptr[ten]
        fstp dword ptr[edx + edi]
        pop edi
        pop esi
        ret 0004
    }
}

const float ScaledSpeed = 1.0;
int NegativeAccelerationEntry = 0x109FDA59;
__declspec(naked) void NegativeAcceleration() {
    static int Return = 0x109FDA5E;
    __asm {
        push    edx
        mov edx, dword ptr[ScaledSpeed]
        jmp dword ptr[Return]
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

int InstaFixPrototypeEntry = 0x10AB8DAA;
__declspec(naked) void InstaFixPrototype() {
    static int InstaFixPrototypeResume = 0x10AB8DE9;
    __asm {
        jmp dword ptr[InstaFixPrototypeResume];
    }
}

static D3DPRESENT_PARAMETERS* overriddenD3dpp;
void ProcessDpp(D3DPRESENT_PARAMETERS* d3dpp) {

    std::cout << "FullScreen_RefreshRateInHz: " << d3dpp->FullScreen_RefreshRateInHz << std::endl;
    std::cout << "BackBufferWidth: " << d3dpp->BackBufferWidth << std::endl;
    std::cout << "AutoDepthStencilFormat: " << d3dpp->AutoDepthStencilFormat << std::endl;
    std::cout << "BackBufferCount: " << d3dpp->BackBufferCount << std::endl;
    std::cout << "BackBufferFormat: " << d3dpp->BackBufferFormat << std::endl;
    std::cout << "BackBufferHeight: " << d3dpp->BackBufferHeight << std::endl;
    std::cout << "EnableAutoDepthStencil: " << d3dpp->EnableAutoDepthStencil << std::endl;
    std::cout << "SwapEffect: " << d3dpp->SwapEffect << std::endl;
    std::cout << "MultiSampleType: " << d3dpp->MultiSampleType << std::endl;
    std::cout << "Flags: " << d3dpp->Flags << std::endl;

    D3DPRESENT_PARAMETERS d3dppReplacement;
    ZeroMemory(&d3dppReplacement, sizeof(d3dppReplacement));
    
    d3dppReplacement.BackBufferWidth = d3dpp->BackBufferWidth;


    overriddenD3dpp = &d3dppReplacement;
}

int DPPEntry = 0x1095CA7A;
__declspec(naked) void DPP() {
    static D3DPRESENT_PARAMETERS* d3dpp;
    __asm {
        add     eax, 0x46A8
        mov dword ptr[d3dpp], eax
        push    eax
        pushad
    }
    ProcessDpp(d3dpp);
    static int Return = 0x1095CA80;
    __asm {
        popad
        jmp dword ptr[Return]
    }
}

#define D3DX_PI    (3.14159265358979323846)

static LPDIRECT3DDEVICE8 pDevice;

void DebugD3D() {
    D3DCAPS8 caps;
    HRESULT result = pDevice->GetDeviceCaps(&caps);
    if (FAILED(result)) {
        std::wcout << "DeviceCaps Failed" << std::endl;
        return;
    }
    std::wcout << std::fixed << std::hex << "DeviceCaps" << std::endl;
    std::wcout << std::fixed << std::hex << "DeviceType: " << caps.DeviceType << std::endl;
    std::wcout << std::fixed << std::hex << "AdapterOrdinal: " << caps.AdapterOrdinal << std::endl;
    std::wcout << std::fixed << std::hex << "Caps: " << caps.Caps << std::endl;
    std::wcout << std::fixed << std::hex << "Caps2: " << caps.Caps2 << std::endl;
    std::wcout << std::fixed << std::hex << "Caps3: " << caps.Caps3 << std::endl;
    std::wcout << std::fixed << std::hex << "PresentationIntervals: " << caps.PresentationIntervals << std::endl;
    std::wcout << std::fixed << std::hex << "CursorCaps: " << caps.CursorCaps << std::endl;
    std::wcout << std::fixed << std::hex << "DevCaps: " << caps.DevCaps << std::endl;
    std::wcout << std::fixed << std::hex << "PrimitiveMiscCaps: " << caps.PrimitiveMiscCaps << std::endl;
    std::wcout << std::fixed << std::hex << "RasterCaps: " << caps.RasterCaps << std::endl;
    std::wcout << std::fixed << std::hex << "ZCmpCaps: " << caps.ZCmpCaps << std::endl;
    std::wcout << std::fixed << std::hex << "SrcBlendCaps: " << caps.SrcBlendCaps << std::endl;
    std::wcout << std::fixed << std::hex << "DestBlendCaps: " << caps.DestBlendCaps << std::endl;
    std::wcout << std::fixed << std::hex << "AlphaCmpCaps: " << caps.AlphaCmpCaps << std::endl;
    std::wcout << std::fixed << std::hex << "ShadeCaps: " << caps.ShadeCaps << std::endl;
    std::wcout << std::fixed << std::hex << "TextureCaps: " << caps.TextureCaps << std::endl;
    std::wcout << std::fixed << std::hex << "TextureFilterCaps: " << caps.TextureFilterCaps << std::endl;          // D3DPTFILTERCAPS for IDirect3DTexture8's
    std::wcout << std::fixed << std::hex << "CubeTextureFilterCaps: " << caps.CubeTextureFilterCaps << std::endl;      // D3DPTFILTERCAPS for IDirect3DCubeTexture8's
    std::wcout << std::fixed << std::hex << "VolumeTextureFilterCaps: " << caps.VolumeTextureFilterCaps << std::endl;    // D3DPTFILTERCAPS for IDirect3DVolumeTexture8's
    std::wcout << std::fixed << std::hex << "TextureAddressCaps: " << caps.TextureAddressCaps << std::endl;         // D3DPTADDRESSCAPS for IDirect3DTexture8's
    std::wcout << std::fixed << std::hex << "VolumeTextureAddressCaps: " << caps.VolumeTextureAddressCaps << std::endl;   // D3DPTADDRESSCAPS for IDirect3DVolumeTexture8's
    std::wcout << std::fixed << std::hex << "LineCaps: " << caps.LineCaps << std::endl;                   // D3DLINECAPS
    std::wcout << std::fixed << std::hex << "MaxTextureWidth: " << caps.MaxTextureWidth << std::endl;
    std::wcout << std::fixed << std::hex << "MaxTextureHeight: " << caps.MaxTextureHeight << std::endl;
    std::wcout << std::fixed << std::hex << "MaxVolumeExtent: " << caps.MaxVolumeExtent << std::endl;
    std::wcout << std::fixed << std::hex << "MaxTextureRepeat: " << caps.MaxTextureRepeat << std::endl;
    std::wcout << std::fixed << std::hex << "MaxTextureAspectRatio: " << caps.MaxTextureAspectRatio << std::endl;
    std::wcout << std::fixed << std::hex << "MaxAnisotropy: " << caps.MaxAnisotropy << std::endl;
    std::wcout << std::fixed << std::hex << "MaxVertexW: " << caps.MaxVertexW << std::endl;
    std::wcout << std::fixed << std::hex << "GuardBandLeft: " << caps.GuardBandLeft << std::endl;
    std::wcout << std::fixed << std::hex << "GuardBandTop: " << caps.GuardBandTop << std::endl;
    std::wcout << std::fixed << std::hex << "GuardBandRight: " << caps.GuardBandRight << std::endl;
    std::wcout << std::fixed << std::hex << "GuardBandBottom: " << caps.GuardBandBottom << std::endl;
    std::wcout << std::fixed << std::hex << "ExtentsAdjust: " << caps.ExtentsAdjust << std::endl;
    std::wcout << std::fixed << std::hex << "StencilCaps: " << caps.StencilCaps << std::endl;
    std::wcout << std::fixed << std::hex << "FVFCaps: " << caps.FVFCaps << std::endl;
    std::wcout << std::fixed << std::hex << "TextureOpCaps: " << caps.TextureOpCaps << std::endl;
    std::wcout << std::fixed << std::hex << "MaxTextureBlendStages: " << caps.MaxTextureBlendStages << std::endl;
    std::wcout << std::fixed << std::hex << "MaxSimultaneousTextures: " << caps.MaxSimultaneousTextures << std::endl;
    std::wcout << std::fixed << std::hex << "VertexProcessingCaps: " << caps.VertexProcessingCaps << std::endl;
    std::wcout << std::fixed << std::hex << "MaxActiveLights: " << caps.MaxActiveLights << std::endl;
    std::wcout << std::fixed << std::hex << "MaxUserClipPlanes: " << caps.MaxUserClipPlanes << std::endl;
    std::wcout << std::fixed << std::hex << "MaxVertexBlendMatrices: " << caps.MaxVertexBlendMatrices << std::endl;
    std::wcout << std::fixed << std::hex << "MaxVertexBlendMatrixIndex: " << caps.MaxVertexBlendMatrixIndex << std::endl;
    std::wcout << std::fixed << std::hex << "MaxPointSize: " << caps.MaxPointSize << std::endl;
    std::wcout << std::fixed << std::hex << "MaxPrimitiveCount: " << caps.MaxPrimitiveCount << std::endl;          // max number of primitives per DrawPrimitive call
    std::wcout << std::fixed << std::hex << "MaxVertexIndex: " << caps.MaxVertexIndex << std::endl;
    std::wcout << std::fixed << std::hex << "MaxStreams: " << caps.MaxStreams << std::endl;
    std::wcout << std::fixed << std::hex << "MaxStreamStride: " << caps.MaxStreamStride << std::endl;            // max stride for SetStreamSource
    std::wcout << std::fixed << std::hex << "VertexShaderVersion: " << caps.VertexShaderVersion << std::endl;
    std::wcout << std::fixed << std::hex << "MaxVertexShaderConst: " << caps.MaxVertexShaderConst << std::endl;       // number of vertex shader constant registers
    std::wcout << std::fixed << std::hex << "PixelShaderVersion: " << caps.PixelShaderVersion << std::endl;

    if (caps.PixelShaderVersion >= D3DPS_VERSION(1, 0))
    {
        int majorVersion = D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion);
        int minorVersion = D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion);
        std::wcout << std::fixed << std::hex << "PixelShaderVersion_Major: " << majorVersion << std::endl;
        std::wcout << std::fixed << std::hex << "PixelShaderVersion_Minor: " << minorVersion << std::endl;
        
    }

    std::wcout << std::fixed << std::hex << "MaxPixelShaderValue: " << caps.MaxPixelShaderValue << std::endl;
}

extern "C" LONG RtlGetVersion(OSVERSIONINFOEXW*);

bool IsWindows10OrGreater() {

    OSVERSIONINFOEXW osInfo = { 0 };
    osInfo.dwOSVersionInfoSize = sizeof(osInfo);

    // Call RtlGetVersion to get the OS version
    if (RtlGetVersion(&osInfo) == 0) {
        return osInfo.dwMajorVersion >= 10;
    }
    
    return false;
}

bool IsSetProcessMitigationPolicySupported() {
    if (IsWindows10OrGreater()) {
        HMODULE hKernel32 = LoadLibrary(L"kernel32.dll");
        if (hKernel32 != NULL) {
            auto SetProcessMitigationPolicy = GetProcAddress(hKernel32, "SetProcessMitigationPolicy");
            FreeLibrary(hKernel32);
            return SetProcessMitigationPolicy != NULL;
        }
    }

    return false;
}

// Enables features which reduce the risks of potential buffer overflow vulernabilities in the base game
void EnableProcessSecurity()
{
    if (Config::security_acg && IsSetProcessMitigationPolicySupported()) {
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
bool initialized = false;
void OnDeviceCreated() {
    if (!initialized) {
        EnableProcessSecurity();
        initialized = true;
    }
}

int DeviceEntry = 0x1095CAAA;
__declspec(naked) void Device() {
    static int Return = 0x1095CAB2;
    __asm {
        mov     eax, [ecx + 0x46A4]
        mov     edx, [eax]
        pushad
        mov     [pDevice], eax
    }
    OnDeviceCreated();
    __asm {
        popad
        jmp     dword ptr[Return]
    }
}
void D3DMatrixIdentity(D3DMATRIX* pOut)
{
    // Zero out the matrix
    std::memset(pOut, 0, sizeof(D3DMATRIX));

    // Set the diagonal to 1
    pOut->m[0][0] = 1.0f;
    pOut->m[1][1] = 1.0f;
    pOut->m[2][2] = 1.0f;
    pOut->m[3][3] = 1.0f;
}

D3DMATRIX* D3DXMatrixPerspectiveFovLH(D3DMATRIX* pOut, float fovy, float aspect, float zn, float zf)
{
    D3DMatrixIdentity(pOut);

    float tanHalfFovy = tanf(fovy / 2.0f);
    pOut->m[0][0] = 1.0f / (aspect * tanHalfFovy);
    pOut->m[1][1] = 1.0f / tanHalfFovy;
    pOut->m[2][2] = zf / (zf - zn);
    pOut->m[2][3] = 1.0f;
    pOut->m[3][2] = (zf * zn) / (zn - zf);
    pOut->m[3][3] = 0.0f;

    return pOut;
}

float RadToDeg(float radians) {
    return radians * (180.0f / D3DX_PI);
}

float DegreesToRadians(float degrees)
{
    return degrees * (D3DX_PI / 180.0f);
}

static float GetOriginalFovY(float m11) {
    return 2.0f * atanf(1.0f / m11);
}

static void ApplyMatrixPerspectiveFovLH(D3DMATRIX* projMatrix, float displayWidth, float displayHeight)
{
    float fovY = GetOriginalFovY(projMatrix->m[1][1]);// DegreesToRadians(57.0f);
    /*auto ogFov = RadToDeg(fovY);
    std::cout << std::format("ogfov:  {:.2f}", ogFov) << std::endl;*/
    float aspectRatio = displayWidth / displayHeight; // Example resolution
    float nearClip = 0.1f;// 1.0f;
    float farClip = 50000.0f;

    D3DXMatrixPerspectiveFovLH(projMatrix, fovY, aspectRatio, nearClip, farClip);
}

void ApplyWidthScaling(D3DMATRIX* projMatrix, float aspectRatio)
{
    if ((projMatrix->_11 < 0.0f) ^ (projMatrix->_22 < 0.0f)) {
        projMatrix->_22 = projMatrix->_11 * aspectRatio;
        projMatrix->_22 *= -1;
    }
    else {
        projMatrix->_22 = projMatrix->_11 * aspectRatio;
    }
}

void ApplyWidthScaling(D3DMATRIX* projMatrix, float displayHeight, float displayWidth)
{
    ApplyWidthScaling(projMatrix, displayWidth / displayHeight);
}

void ApplyHeightScaling(D3DMATRIX* projMatrix, D3DDISPLAYMODE& d3dDisplayMode)
{
    if ((projMatrix->_11 < 0.0f) ^ (projMatrix->_22 < 0.0f)) {
        projMatrix->_11 = (projMatrix->_22 / d3dDisplayMode.Width) * d3dDisplayMode.Height;
        projMatrix->_11 *= -1;
    }
    else {
        projMatrix->_11 = (projMatrix->_22 / d3dDisplayMode.Width) * d3dDisplayMode.Height;
    }
}

static bool isMercEnhancedRealityStationary = 0;

static int MercEnhancedRealityStationaryEntry = 0x10AF8069;
__declspec(naked) void MercEnhancedRealityStationary() {
    static int Return = 0x10AF8072;
    static int MercErStationary = 0x10AF8C00;
    isMercEnhancedRealityStationary = true;
    __asm {
        push    0
        push ebx
        push esi
        call dword ptr[MercErStationary]

    }

    isMercEnhancedRealityStationary = false;
    __asm {
        jmp dword ptr[Return]
    }
}

static bool isSpyEnhancedRealityStationary = 0;
static int SpyEnhancedRealityStationaryEntry = 0x10A90ED2;
__declspec(naked) void SpyEnhancedRealityStationary() {
    static int Return = 0x10A90EDB;
    static int MercErStationary = 0x10AF8C00;
    isSpyEnhancedRealityStationary = true;
    __asm {
        push    1
        push esi
        push ebx
        call dword ptr[MercErStationary]

    }
    isSpyEnhancedRealityStationary = false;
    __asm {
        jmp dword ptr[Return]
    }
}

static int FixMovingEnhancedRealityScalingEntry = 0x10AF8CD0;
__declspec(naked) void FixMovingEnhancedRealityScaling() {
    static int Return = 0x10AF8CDA;
    __asm {
        mov[esp + 0x64], 1//eax
        mov eax, [edi + 0x000001A0]
        jmp dword ptr[Return]
    }
}

static void SetupProjectionMatrix(D3DMATRIX* projMatrix)
{
    D3DDISPLAYMODE d3dDisplayMode;
    pDevice->GetDisplayMode(&d3dDisplayMode);

    float displayHeight = static_cast<float>(d3dDisplayMode.Height);
    float displayWidth = static_cast<float>(d3dDisplayMode.Width);
    displayWidth = min(displayWidth, d3dDisplayMode.Height * (16.0f / 9.0));
    //float displayAspectRatio = displayHeight/ displayWidth;
    auto renderAspectRatio = fabs(roundf((projMatrix->_11 / projMatrix->_22) * 100.0f) / 100.0f);
    const float fourByThreeAspect = 0.75f;
    if (renderAspectRatio == fourByThreeAspect) {
        /*std::string message = std::format("deg:  {:.2f}", RadToDeg(originalFov);
        std::cout << message << std::endl;*/
        if (fabs(projMatrix->_22) < 0.1f) {
            if (!isMercEnhancedRealityStationary && !isSpyEnhancedRealityStationary) {
                return;
            }

            auto newAspect = (displayWidth / displayHeight);
            ApplyWidthScaling(projMatrix, newAspect);
                
            projMatrix->_42 = (projMatrix->_42 / (4.0 / 3)) * newAspect;
            return;
        }

        int scalingMode = 0;
        
        switch (scalingMode) {
        default:
            ApplyWidthScaling(projMatrix, displayHeight, displayWidth);
            break;
        case 1:
            ApplyMatrixPerspectiveFovLH(projMatrix, displayWidth, displayHeight);
            break;
        case 2:
            ApplyHeightScaling(projMatrix, d3dDisplayMode);
            break;
        }
    }
}
int SetProjection1Entry = 0x1096CA07;
__declspec(naked) void SetProjection1() {
    static D3DMATRIX* projMatrix;
    static int Return = 0x1096CA0D;
    __asm {
        pushad
        mov     dword ptr[projMatrix], ebp
    }
    SetupProjectionMatrix(projMatrix);
   
    __asm {
        popad
        call    dword ptr[ecx + 0x94]
        jmp     dword ptr[Return]
    }
}

int SetProjection2Entry = 0x1097827B;
__declspec(naked) void SetProjection2() {
    static D3DMATRIX* projMatrix;
    static int Return = 0x10978281;
    __asm {
        pushad
        mov     dword ptr[projMatrix], edx
    }
    SetupProjectionMatrix(projMatrix);

    __asm {
        popad
        call    dword ptr[ecx + 0x94]
        jmp     dword ptr[Return]
    }
}

int SetProjection3Entry = 0x1096BC07; 
__declspec(naked) void SetProjection3() {
    static D3DMATRIX* projMatrix;
    static int Return = 0x1096BC0D;
    __asm {
        pushad
        mov eax, dword ptr[0x10CC9560]
        mov     dword ptr[projMatrix], eax
    }
    SetupProjectionMatrix(projMatrix);

    __asm {
        popad
        call    dword ptr[ecx + 0x94]
        jmp     dword ptr[Return]
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
        to.sin_addr.s_addr = inet_addr(WStringToString(Config::directConnectIp).c_str());
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

static int removeClientFpsCapEntry = 0x1090801C;
__declspec(naked) void removeClientFpsCap() {
    static int Return = 0x10908063;
    __asm {
        jmp dword ptr[Return]
    }
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

std::chrono::steady_clock::time_point nextFrameTime;
std::chrono::steady_clock::time_point lastFrameTime;
void UpdateLastFrameRenderedTime() {
    double frameRateLimit;
    if (IsListenServer()) {
        frameRateLimit = (double)Config::frameRateLimit_hosting;
    }
    else {
        frameRateLimit = (double)Config::frameRateLimit_client;
    }
    double frameTimeSeconds = (double)1.0 / frameRateLimit;
    auto frameTimeNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::duration<double>((double)1.0 / frameRateLimit)
    ).count();

    lastFrameTime = nextFrameTime;
    nextFrameTime = std::chrono::high_resolution_clock::now() + std::chrono::nanoseconds(frameTimeNanoseconds);
}

void preciseSpin() {
    while (std::chrono::high_resolution_clock::now() < nextFrameTime) {
    }    
}

void preciseSleep() {
    auto remainingFrameTime = nextFrameTime - std::chrono::high_resolution_clock::now();
    auto remainingMS = std::chrono::duration<double, std::milli>(remainingFrameTime).count();
    if (remainingMS - std::floor(remainingMS) >= 0.5) {
        remainingMS = std::floor(remainingMS);
    }
    else {
        remainingMS = std::floor(remainingMS) - 1;
    }
    if (remainingMS > 0) {
        timeBeginPeriod(1);
        Sleep(static_cast<DWORD>(remainingMS));
        timeEndPeriod(1);
    }

    preciseSpin();
}

void LimitFrameRate() {
    switch (Config::frameTimingMode) {
    default:
        preciseSpin();
        break;
    case 1:
        preciseSleep();
        break;
    }

    UpdateLastFrameRenderedTime();
}

double ConvertFOV(double horizontalFOV, double newAspectRatio) {
    double horizontalFOVRad = horizontalFOV * D3DX_PI / 180.0;
    double originalAspectRatioWidth = 4.0;
    double originalAspectRatioHeight = 3.0;
    double verticalFOVRad = 2 * atan(tan(horizontalFOVRad / 2) * originalAspectRatioHeight / originalAspectRatioWidth);
    double verticalFOV = verticalFOVRad * 180.0 / D3DX_PI;
    double newAspectRatioWidth = newAspectRatio;
    double newHorizontalFOVRad = 2 * atan(tan(verticalFOVRad / 2) * newAspectRatioWidth);
    double newHorizontalFOV = newHorizontalFOVRad * 180.0 / D3DX_PI;

    return min(newHorizontalFOV, Config::widescreenFovCap);
}

float displayHeightLast = 0;
float displayWidthLast = 0;
float originalSfv = 0.0;
float originalDfv = 0.0;
float hSfv = 0.0;
float hDfv = 0.0;
void WidescreenViewFix() {
    if (lvIn != NULL && &lvIn->lPlC() != NULL && pDevice != NULL) {
        // Copy/pasted.  TODO: Refactor
        D3DDISPLAYMODE d3dDisplayMode;
        pDevice->GetDisplayMode(&d3dDisplayMode);
        float defv = lvIn->lPlC().Defv();
        float displayHeight = static_cast<float>(d3dDisplayMode.Height);
        float displayWidth = static_cast<float>(d3dDisplayMode.Width);
        displayWidth = min(displayWidth, d3dDisplayMode.Height * (16.0f / 9.0));

        if (displayWidthLast != displayWidth || displayHeightLast != displayHeight || (defv != hSfv && defv != hDfv)) {
            if (originalSfv == 0.0) {
                originalSfv = lvIn->lPlC().Sfv();
                originalDfv = lvIn->lPlC().Dfv();
            }

            bool wasSfv = lvIn->lPlC().Defv() == hSfv;
            bool wasDfv = lvIn->lPlC().Defv() == hDfv;

            auto aspectRatio = displayWidth / displayHeight;
            aspectRatioMenuVertMouseInputMultiplier = aspectRatio/(4.0 / 3.0);
            hSfv = ConvertFOV(originalSfv, aspectRatio);
            hDfv = ConvertFOV(originalDfv, aspectRatio);

            if (wasSfv) {
                lvIn->lPlC().Defv() = hSfv;
            }
            else if (wasDfv) {
                lvIn->lPlC().Defv() = hDfv;
            }

            // last calculated
            displayHeightLast = displayHeight;
            displayWidthLast = displayWidth;
        }

        lvIn->lPlC().Sfv() = hSfv;
        lvIn->lPlC().Dfv() = hDfv;
    }
}

int viewFixEntry = 0x1095E417;
int viewFix2Entry = 0x1095E43C;
__declspec(naked) void viewFix() {
    __asm {
        pushad
    }
    WidescreenViewFix();
    __asm {
        popad
        add     esp, 0x14
        retn    0x4
    }
}

int alternativeFrameModeEntry = 0x109EC82F;
__declspec(naked) void alternativeFrameMode() {
    __asm {
        pushad
    }
    LimitFrameRate();
    __asm {
        popad
        retn    0x10
    }
}


int startFrameTimerEntry = 0x1095E331;
__declspec(naked) void beforePresent() {
    static int Return = 0x1095E38D;
    __asm {
        mov edx, [esp + 00]
        jmp dword ptr[Return]
    }

}

// TODO: Remove this approach. Inferior to the others
//int fixSleepTimerEntry = 0x1095E340;
//const int fixSleepTimerReturn = 0x1095E38D;
//__declspec(naked) void fixSleepTimer() {
//    static int frameRateLimit;
//    __asm {
//        pushad
//    }
//    if (IsListenServer()) {
//        frameRateLimit = Config::frameRateLimit_hosting;
//    }
//    else {
//        frameRateLimit = Config::frameRateLimit_client;
//    }
//    __asm {
//        popad
//        mov     eax, dword ptr[frameRateLimit]
//        fild    dword ptr[frameRateLimit]
//        test    eax, eax
//        jge     loc_1095E356
//        mov     eax, 0x10C10A24
//        fadd    dword ptr[eax]
//        loc_1095E356:
//        fdivr dword ptr[one]
//        fld     st(1)
//        fcomp   st(1)
//        fnstsw  ax
//        test    ah, 0x05
//        jp      loc_1095E389
//        fsub    st(0), st(1)
//        fmul    qword ptr[oneThousand]
//        call    dword ptr[ToMilliseconds]
//        fstp    st(0)
//        push    eax
//
//        push    0x1
//        mov     eax, dword ptr[timeBeginPeriodAddr]
//        call    dword ptr[eax]
//
//        mov     eax, dword ptr[sleep]
//        call    dword ptr[eax]
//
//        push    0x1
//        mov     eax, dword ptr[timeEndPeriodAddr]
//        call    dword ptr[eax]
//
//        mov     edx, [esp + 00]
//        jmp     fixSleepTimerEnd
//
//        loc_1095E389 :
//        fstp    st(0)
//        fstp    st(0)
//
//        fixSleepTimerEnd :
//        jmp     dword ptr[fixSleepTimerReturn]
//    }
//}

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

int animatedTextureFixEntry = 0x109F2561;
__declspec(naked) void animatedTextureFix() {
    __asm {
        PUSH    ESI

        MOV     ESI, 0x10CCADA0 // not precise
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
    logger_->log(L"name: " + unicodeString);
    logger_->log("id: " + std::to_string(id));
    logger_->log("hexid: " + toHexString(id));
    logger_->log("hexidoffset: " + toHexString(id * 4));
    logger_->log("flags: " + toHexString(flags));
    logger_->log("unknown: " + toHexString(unknown));
    logger_->log("addr: " + toHexString(addr));

    
    logger_->log("");
}

static int ScctEnhancedIdentifier = 0x10C42DA4;
static int AddEnhancedGuiResolutionsEntry = 0x10B0FC5E;
__declspec(naked) void AddEnhancedGuiResolutions() {
    static int Return = 0x10B0FC64;
    __asm {
        push eax
        mov eax, dword ptr[ScctEnhancedIdentifier]
        mov al, byte ptr [eax]
        cmp al, '3'
        jne notEnhanced

        pop eax
        push 13 // resolution count

        notEnhanced:
        push 0x10C42C74
        jmp dword ptr[Return]
    }
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

//static int RoundFpuFunction = 0x10B83AA0;
int MenuMouseSensitivityYEntry = 0x10A56B2D;
__declspec(naked) void MenuMouseSensitivityY() {
    static int Return = 0x10A56B44;
    //static float yRemainder = 0.0;
    __asm {
        /*fild dword ptr[yMouseDelta]
        fchs

        fmul dword ptr[aspectRatioMenuVertMouseInputMultiplier]
        fmul dword ptr[Config::menuSensitivity]
        fsubp st(1), st(0)
        fadd dword ptr[yRemainder]
        fst dword ptr[yRemainder]
        call dword ptr[RoundFpuFunction]
        mov[esi + 0x00000CD0], eax
        fld dword ptr[yRemainder]
        fisub dword ptr[esi + 0x00000CD0]
        fstp dword ptr[yRemainder]*/
        fld dword ptr[menuPositionY]
        fistp dword ptr[esi + 0x00000CD0]

        jmp dword ptr[Return]
    }
}

int MenuMouseSensitivityXEntry = 0x10A56A76;
__declspec(naked) void MenuMouseSensitivityX() {
    static int Return = 0x10A56A98;
    //static float xRemainder = 0.0;
    __asm {
        fild dword ptr[xMouseDelta]
        add esp,0x4
        fst     dword ptr[esp + 0x1C]

       /* fmul dword ptr[Config::menuSensitivity]
        fiadd[esi + 0x00000CCC]
        fadd dword ptr[xRemainder]
        fst dword ptr[xRemainder]
        call dword ptr[RoundFpuFunction]
        mov[esi + 0x00000CCC], eax
        fld dword ptr[xRemainder]
        fisub dword ptr[esi + 0x00000CCC]
        fstp dword ptr[xRemainder]*/
        fld dword ptr[menuPositionX]
        fistp dword ptr[esi + 0x00000CCC]

        jmp dword ptr[Return]
    }
}

bool __cdecl WriteBytes(uintptr_t targetAddress, const uint8_t* bytes, size_t length) {
    logger_->log("Writing bytes at " + toHexString(targetAddress));

    DWORD oldProtect;
    if (!VirtualProtect(reinterpret_cast<LPVOID>(targetAddress), length, PAGE_READWRITE, &oldProtect)) {
        logger_->log("Failed to change memory protection");
        return false;
    }

    memcpy(reinterpret_cast<void*>(targetAddress), bytes, length);
    if (!VirtualProtect(reinterpret_cast<LPVOID>(targetAddress), length, oldProtect, &oldProtect)) {
        logger_->log("Failed to restore memory protection");
        return false;
    }

    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(targetAddress), length);
    logger_->log("Finished writing bytes at " + toHexString(targetAddress));
    return true;
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

    WriteJump(startFrameTimerEntry, beforePresent);
    WriteJump(alternativeFrameModeEntry, alternativeFrameMode);
    if (Config::frameRateLimit_client_unlock)
        WriteJump(removeClientFpsCapEntry, removeClientFpsCap);

    WriteJump(sendBroadcastLanMessageEntry, sendBroadcastLanMessage);
    WriteJump(ServerInfoBroadcastEntry, ServerInfoBroadcast);
    WriteJump(InstaFixPrototypeEntry, InstaFixPrototype);
    WriteJump(DeviceEntry, Device);

    if (Config::widescreenAspectRatioFix) {
        WriteJump(SetProjection1Entry, SetProjection1);
        WriteJump(SetProjection2Entry, SetProjection2);
        // May be redundant
        WriteJump(SetProjection3Entry, SetProjection3);

        WriteJump(viewFixEntry, viewFix);
        WriteJump(viewFix2Entry, viewFix);
        WriteJump(MercEnhancedRealityStationaryEntry, MercEnhancedRealityStationary);
        WriteJump(SpyEnhancedRealityStationaryEntry, SpyEnhancedRealityStationary);
        WriteJump(FixMovingEnhancedRealityScalingEntry, FixMovingEnhancedRealityScaling);
    }

    WriteJump(DPPEntry, DPP);
    WriteJump(SetLvInEntry, SetLvIn);
    WriteJump(AddEnhancedGuiResolutionsEntry, AddEnhancedGuiResolutions);
    WriteJump(OnStateChangeEntry, OnStateChange);

    if (Config::mouseInputFix) {
        WriteJump(DisableMouseInputEntry, DisableMouseInput);
        WriteJump(FixMouseInputEntry, FixMouseInput);
        WriteJump(X_WriteMouseInputEntry, X_WriteMouseInput);
        WriteJump(Y_WriteMouseInputEntry, Y_WriteMouseInput);
        WriteJump(MenuMouseSensitivityYEntry, MenuMouseSensitivityY);
        WriteJump(MenuMouseSensitivityXEntry, MenuMouseSensitivityX);
        WriteJump(BaseMouseSensitivityEntry, BaseMouseSensitivity);
        WriteJump(NegativeAccelerationEntry, NegativeAcceleration);
    }

    if (Config::disableStickyCamContextMenu) {
        /*WriteJump(StickyCamContextMenuBlock1Entry, StickyCamContextMenuBlock1);
        WriteJump(StickyCamContextMenuBlock2Entry, StickyCamContextMenuBlock2);*/
        WriteJump(StickyCamContextMenuBlock3Entry, StickyCamContextMenuBlock3);
        
        uint8_t shortJump[] = { 0xEB };
        WriteBytes(0x10B2CE1B, shortJump, sizeof(shortJump));
    }
    //WriteJump(unrealScriptNameDefinitionLookupEntry, unrealScriptNameDefinitionLookup);
    //WriteJump(0x1093B590, test);
}