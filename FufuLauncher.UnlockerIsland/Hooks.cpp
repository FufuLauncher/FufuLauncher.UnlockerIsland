#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Hooks.h"
#include "Scanner.h"
#include "Config.h"
#include "Utils.h"
#include "MinHook/MinHook.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include <iostream>
#include <atomic>
#include <mutex>
#include <string>
#include <d3d11.h>
#include <processthreadsapi.h>
#include <ctime>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <winsock2.h>
#include <wincodec.h>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "MinHook/libMinHook.x64.lib")
#pragma comment(lib, "ws2_32.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class XorString {
    static constexpr char key = 0x5F;

public:
    template<size_t N>
    struct EncryptedData {
        char data[N];
    };

    template<size_t N>
    static constexpr auto encrypt(const char(&str)[N]) {
        EncryptedData<N> encrypted{};
        for (size_t i = 0; i < N; ++i) {
            encrypted.data[i] = str[i] ^ key;
        }
        return encrypted;
    }

    template<size_t N>
    static std::string decrypt(const EncryptedData<N>& encrypted) {
        std::string decrypted;
        decrypted.resize(N - 1);
        for (size_t i = 0; i < N - 1; ++i) {
            decrypted[i] = encrypted.data[i] ^ key;
        }
        return decrypted;
    }
};

const char* GetRegName(int index) {
    static const char* regs[] = { "RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15" };
    if (index >= 0 && index < 16) return regs[index];
    return "???";
}

std::string GetOwnDllDir() {
    char path[MAX_PATH];
    HMODULE hm = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)Hooks::Init, &hm)) {
        GetModuleFileNameA(hm, path, sizeof(path));
        std::string fullPath = path;
        size_t lastSlash = fullPath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            return fullPath.substr(0, lastSlash);
        }
    }
    return ".";
}

std::string GetInstructionInfo(uint8_t* addr) {
    if (!addr) return "";
    std::stringstream ss;
    
    uint8_t b0 = addr[0];
    uint8_t b1 = addr[1];
    uint8_t b2 = addr[2];

    bool isRex = (b0 >= 0x40 && b0 <= 0x4F);
    uint8_t rex = isRex ? b0 : 0;
    uint8_t opcode = isRex ? b1 : b0;
    uint8_t modrm = isRex ? b2 : b1;
    
    int regIndex = ((modrm >> 3) & 7);
    if (rex & 4) regIndex += 8;
    
    if (opcode == 0xE8) {
        ss << "CALL (Rel)";
    }
    else if (opcode == 0xE9) {
        ss << "JMP (Rel)";
    }
    else if (opcode == 0x8B) {
        ss << "MOV " << GetRegName(regIndex);
    }
    else if (opcode == 0x8D) {
        ss << "LEA " << GetRegName(regIndex);
    }
    else if (opcode == 0x33) {
        ss << "XOR " << GetRegName(regIndex);
    }
    else if (opcode == 0x89) {
        ss << "MOV [Mem], " << GetRegName(regIndex);
    }
    else {
        ss << "OP: " << std::hex << std::uppercase << (int)opcode;
    }
    
    ss << " | Bytes: ";
    for (int i = 0; i < 5; ++i) {
        ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)addr[i] << " ";
    }

    return ss.str();
}

namespace EncryptedPatterns {
    // 1. GetFrameCount
    constexpr auto GetFrameCount = XorString::encrypt("E8 ? ? ? ? 85 C0 7E 0E E8 ? ? ? ? 0F 57 C0 F3 0F 2A C0 EB 08");
    // 2. SetFrameCount
    constexpr auto SetFrameCount = XorString::encrypt("E8 ? ? ? ? E8 ? ? ? ? 83 F8 1F 0F 9C 05 ? ? ? ? 48 8B 05");
    // 3. ChangeFOV
    constexpr auto ChangeFOV = XorString::encrypt("40 53 48 83 EC 60 0F 29 74 24 ? 48 8B D9 0F 28 F1 E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? E8 ? ? ? ? 48 8B C8");
    // 4. SwitchInput
    constexpr auto SwitchInput = XorString::encrypt("56 57 48 83 EC ? 48 89 CE 80 3D ? ? ? ? 00 48 8B 05 ? ? ? ? 0F 85 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9 0F 84 ? ? ? ? 48 8B 15 ? ? ? ? E8 ? ? ? ? 48 89 C7 48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9 0F 84 ? ? ? ? 31 D2");
    // 5. QuestBanner
    constexpr auto QuestBanner = XorString::encrypt("41 57 41 56 56 57 55 53 48 81 EC ? ? ? ? 0F 29 BC 24 ? ? ? ? 0F 29 B4 24 ? ? ? ? 48 89 CE 80 3D ? ? ? ? 00 0F 85 ? ? ? ? 48 8B 96");
    // 6. FindGameObject
    constexpr auto FindGameObject = XorString::encrypt("E9 ? ? ? ? 66 66 2E 0F 1F 84 00 ? ? ? ? E9 ? ? ? ? 66 66 2E 0F 1F 84 00 ? ? ? ? 48 83 EC ? C7 44 24 ? 00 00 00 00 48 8D 54 24");
    // 7. SetActive
    constexpr auto SetActive = XorString::encrypt("E9 ? ? ? ? 66 66 2E 0F 1F 84 00 ? ? ? ? E9 ? ? ? ? 66 66 2E 0F 1F 84 00 ? ? ? ? E9 ? ? ? ? 66 66 2E 0F 1F 84 00 ? ? ? ? E9 ? ? ? ? 66 66 2E 0F 1F 84 00 ? ? ? ? E9 ? ? ? ? 66 66 2E 0F 1F 84 00 ? ? ? ? E9 ? ? ? ? 66 66 2E 0F 1F 84 00 ? ? ? ? E9 ? ? ? ? 66 66 2E 0F 1F 84 00 ? ? ? ? E9 ? ? ? ? 66 66 2E 0F 1F 84 00 ? ? ? ? E9 ? ? ? ? 66 66 2E 0F 1F 84 00 ? ? ? ? 45 31 C9");
    // 8. DamageText
    constexpr auto DamageText = XorString::encrypt("41 57 41 56 41 55 41 54 56 57 55 53 48 81 EC ? ? ? ? 44 0F 29 9C 24 ? ? ? ? 44 0F 29 94 24 ? ? ? ? 44 0F 29 8C 24 ? ? ? ? 44 0F 29 84 24 ? ? ? ? 0F 29 BC 24 ? ? ? ? 0F 29 B4 24 ? ? ? ? 44 89 CF 45 89 C4");
    // 9. EventCamera
    constexpr auto EventCamera = XorString::encrypt("41 57 41 56 56 57 55 53 48 83 EC ? 48 89 D7 49 89 CE 80 3D ? ? ? ? 00 0F 85 ? ? ? ? 80 3D ? ? ? ? 00");
    // 10. FindString
    constexpr auto FindString = XorString::encrypt("56 48 83 ec 20 48 89 ce e8 ? ? ? ? 48 89 f1 89 c2 48 83 c4 20 5e e9 ? ? ? ? cc cc cc cc");
    // 11. CraftPartner
    constexpr auto CraftPartner = XorString::encrypt("41 57 41 56 41 55 41 54 56 57 55 53 48 81 EC ? ? ? ? 4D 89 ? 4C 89 C6 49 89 D4 49 89 CE");
    // 12. CraftEntry
    constexpr auto CraftEntry = XorString::encrypt("41 56 56 57 53 48 83 EC 58 49 89 CE 80 3D ? ? ? ? 00 0F 84 ? ? ? ? 80 3D ? ? ? ? 00 48 8B 0D ? ? ? ? 0F 85");
    // 13. CheckCanEnter
    constexpr auto CheckCanEnter = XorString::encrypt("56 48 81 ec 80 00 00 00 80 3d ? ? ? ? 00 0f 84 ? ? ? ? 80 3d ? ? ? ? 00");
    // 14. OpenTeamPage
    constexpr auto OpenTeamPage = XorString::encrypt("56 57 53 48 83 ec 20 89 cb 80 3d ? ? ? ? 00 74 7a 80 3d ? ? ? ? 00 48 8b 05");
    // 15. OpenTeam
    constexpr auto OpenTeam = XorString::encrypt("48 83 EC ? 80 3D ? ? ? ? 00 75 ? 48 8B 0D ? ? ? ? 80 B9 ? ? ? ? 00 0F 84 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 84 C0 75");
    // 16. DisplayFog
    constexpr auto DisplayFog = XorString::encrypt("0F B6 02 88 01 8B 42 04 89 41 04 F3 0F 10 52 ? F3 0F 10 4A ? F3 0F 10 42 ? 8B 42 08");
    // 17. PlayerPerspective
    constexpr auto PlayerPerspective = XorString::encrypt("E8 ? ? ? ? 48 8B BE ? ? ? ? 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 80 BE ? ? ? ? ? 74 11");
    // 18. SetSyncCount
    constexpr auto SetSyncCount = XorString::encrypt("E8 ? ? ? ? E8 ? ? ? ? 89 C6 E8 ? ? ? ? 31 C9 89 F2 49 89 C0 E8 ? ? ? ? 48 89 C6 48 8B 0D ? ? ? ? 80 B9 ? ? ? ? ? 74 47 48 8B 3D ? ? ? ? 48 85 DF 74 4C");
    // 19. GameUpdate
    constexpr auto GameUpdate = XorString::encrypt("E8 ? ? ? ? 48 8D 4C 24 ? 8B F8 FF 15 ? ? ? ? E8 ? ? ? ?");
    // HSR
    // 1. FPS 1
    constexpr auto HSR_FPS_1 = XorString::encrypt("80 B9 ? ? ? ? 00 0F 84 ? ? ? ? C7 05 ? ? ? ? 03 00 00 00 48 83 C4 20 5E C3");
    // 2. FPS 2
    constexpr auto HSR_FPS_2 = XorString::encrypt("80 B9 ? ? ? ? 00 74 ? C7 05 ? ? ? ? 03 00 00 00 48 83 C4 20 5E C3");
    // 3. FPS 3
    constexpr auto HSR_FPS_3 = XorString::encrypt("75 05 E8 ? ? ? ? C7 05 ? ? ? ? 03 00 00 00 48 83 C4 28 C3");
}

namespace EncryptedStrings {
    constexpr auto SynthesisPage = XorString::encrypt("SynthesisPage");
    constexpr auto QuestBannerPath = XorString::encrypt("Canvas/Pages/InLevelMapPage/GrpMap/GrpPointTips/Layout/QuestBanner");
}

typedef int32_t (WINAPI *tGetFrameCount)();
typedef int32_t (WINAPI *tSetFrameCount)(int32_t);
typedef void (WINAPI *tSwitchInput)(void*);
typedef int32_t (WINAPI *tChangeFov)(void*, float);
typedef void (WINAPI *tSetupQuestBanner)(void*);
typedef void (WINAPI *tShowDamage)(void*, int, int, int, float, Il2CppString*, void*, void*, int);
typedef void (WINAPI *tCraftEntry)(void*);
typedef bool (WINAPI *tCraftPartner)(Il2CppString*, void*, void*, void*, void*);
typedef Il2CppString* (WINAPI *tFindString)(const char*);
typedef void* (WINAPI *tFindGameObject)(Il2CppString*);
typedef void (WINAPI *tSetActive)(void*, bool);
typedef bool (WINAPI *tEventCamera)(void*, void*);
typedef bool (WINAPI *tCheckCanEnter)();
typedef void (WINAPI *tOpenTeamPage)(bool);
typedef void (WINAPI *tOpenTeam)();
typedef __int64 (*tDisplayFog)(__int64, __int64);
typedef void* (WINAPI *tPlayerPerspective)(void*, float, void*);
typedef int32_t (WINAPI *tSetSyncCount)(bool);
typedef __int64 (WINAPI *tGameUpdate)(__int64, const char*);
typedef HRESULT(__stdcall* tPresent)(IDXGISwapChain*, UINT, UINT);
typedef BOOL (WINAPI* tQueryPerformanceCounter)(LARGE_INTEGER*);
typedef ULONGLONG (WINAPI* tGetTickCount64)();
typedef int (WSAAPI* tSend)(SOCKET s, const char* buf, int len, int flags);
typedef int (WSAAPI* tSendTo)(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);

namespace {
    std::atomic<void*> o_GetFrameCount{ nullptr };
    std::atomic<void*> o_SetFrameCount{ nullptr };
    std::atomic<void*> o_ChangeFov{ nullptr };
    std::atomic<void*> o_SetupQuestBanner{ nullptr };
    std::atomic<void*> o_ShowDamage{ nullptr };
    std::atomic<void*> o_CraftEntry{ nullptr };
    std::atomic<void*> o_EventCamera{ nullptr };
    std::atomic<void*> o_OpenTeam{ nullptr };
    std::atomic<void*> o_DisplayFog{ nullptr };
    std::atomic<void*> p_SwitchInput{ nullptr };
    std::atomic<void*> p_FindString{ nullptr };
    std::atomic<void*> p_CraftPartner{ nullptr };
    std::atomic<void*> p_FindGameObject{ nullptr };
    std::atomic<void*> p_SetActive{ nullptr };
    std::atomic<void*> p_CheckCanEnter{ nullptr };
    std::atomic<void*> p_OpenTeamPage{ nullptr };
    std::atomic g_GameUpdateInit{ false };
    std::atomic g_RequestCraft{ false };
    std::atomic<void*> o_PlayerPerspective{ nullptr };
    std::once_flag g_TouchInitOnce;
    std::atomic<void*> o_SetSyncCount{ nullptr };
    std::atomic<void*> o_GameUpdate{ nullptr };
    std::atomic g_RequestReloadPopup{ false };
    std::atomic<void*> p_HSRFpsAddr{ nullptr };
    tPresent o_Present = nullptr;
    ID3D11Device* g_pd3dDevice = nullptr;
    ID3D11DeviceContext* g_pd3dContext = nullptr;
    ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
    HWND g_hGameWindow_ImGui = nullptr;
    bool g_dx11Init = false;
    ID3D11ShaderResourceView* g_LogoTexture = nullptr;
    int g_LogoWidth = 0;
    int g_LogoHeight = 0;
    ImFont* g_fontBold = nullptr;
    tQueryPerformanceCounter o_QueryPerformanceCounter = nullptr;
    tGetTickCount64 o_GetTickCount64 = nullptr;
    std::mutex g_TimeMutex;
    LARGE_INTEGER g_LastRealTimeQPC = { 0 };
    std::atomic<void*> o_send{ nullptr };
    std::atomic<void*> o_sendto{ nullptr };
}


#define HOOK_REL(name, enc_pat, hookFn, storeOrig) \
    { \
        std::cout << "[SCAN] " << name << "..." << std::endl; \
        std::string _dec_pat = XorString::decrypt(enc_pat); \
        void* addr = Scanner::ScanMainMod(_dec_pat); \
        if (addr) { \
            void* target = Scanner::ResolveRelative(addr, 1, 5); \
            if (target) { \
                LogOffset(name, target, addr); \
                std::cout << "   -> Found at: " << target << std::endl; \
                if (MH_CreateHook(target, (void*)hookFn, (void**)&storeOrig) == MH_OK) \
                    std::cout << "   -> Hook Ready." << std::endl; \
                else std::cout << "   -> [ERR] MH_CreateHook Failed." << std::endl; \
            } else std::cout << "   -> [ERR] ResolveRelative Failed." << std::endl; \
        } else std::cout << "   -> [ERR] Pattern Not Found." << std::endl; \
    }

#define HOOK_DIR(name, enc_pat, hookFn, storeOrig) \
    { \
        std::cout << "[SCAN] " << name << "..." << std::endl; \
        std::string _dec_pat = XorString::decrypt(enc_pat); \
        void* addr = Scanner::ScanMainMod(_dec_pat); \
        if (addr) { \
            LogOffset(name, addr, addr); \
            std::cout << "   -> Found at: " << addr << std::endl; \
            if (MH_CreateHook(addr, (void*)hookFn, (void**)&storeOrig) == MH_OK) \
                 std::cout << "   -> Hook Ready." << std::endl; \
            else std::cout << "   -> [ERR] MH_CreateHook Failed." << std::endl; \
        } else std::cout << "   -> [ERR] Pattern Not Found." << std::endl; \
    }

#define SCAN_REL(name, enc_pat, storePtr) \
    { \
        std::cout << "[SCAN] " << name << "..." << std::endl; \
        std::string _dec_pat = XorString::decrypt(enc_pat); \
        void* addr = Scanner::ScanMainMod(_dec_pat); \
        if (addr) { \
            void* target = Scanner::ResolveRelative(addr, 1, 5); \
            if (target) { \
                LogOffset(name, target, addr); \
                storePtr.store(target); \
                std::cout << "   -> Found." << std::endl; \
            } \
        } else std::cout << "   -> [ERR] Not Found." << std::endl; \
    }

#define SCAN_DIR(name, enc_pat, storePtr) \
    { \
        std::cout << "[SCAN] " << name << "..." << std::endl; \
        std::string _dec_pat = XorString::decrypt(enc_pat); \
        void* addr = Scanner::ScanMainMod(_dec_pat); \
        if (addr) { \
            LogOffset(name, addr, addr); \
            storePtr.store(addr); \
            std::cout << "   -> Found." << std::endl; \
        } \
        else std::cout << "   -> [ERR] Not Found." << std::endl; \
    }

struct SafeFogBuffer {
    __declspec(align(16)) uint8_t data[64];
    uint8_t padding[192];
};

static SafeFogBuffer g_fogBuf = { 0 };

int WSAAPI hk_send(SOCKET s, const char* buf, int len, int flags) {
    if (Config::Get().enable_network_toggle && Config::Get().is_currently_blocking) {
        return len; 
    }
    return ((tSend)o_send.load())(s, buf, len, flags);
}

int WSAAPI hk_sendto(SOCKET s, const char* buf, int len, int flags, const struct ::sockaddr* to, int tolen) {
    if (Config::Get().enable_network_toggle && Config::Get().is_currently_blocking) {
        return len; 
    }
    return ((tSendTo)o_sendto.load())(s, buf, len, flags, to, tolen);
}

BOOL WINAPI hk_QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount) {
    if (!o_QueryPerformanceCounter(&g_LastRealTimeQPC)) return FALSE; 

    static LARGE_INTEGER s_LastReal = { 0 };
    static LARGE_INTEGER s_LastFake = { 0 };

    std::lock_guard lock(g_TimeMutex);

    if (s_LastReal.QuadPart == 0) {
        s_LastReal = g_LastRealTimeQPC;
        s_LastFake = g_LastRealTimeQPC;
    }

    if (Config::Get().enable_speedhack) {
        double delta = (double)(g_LastRealTimeQPC.QuadPart - s_LastReal.QuadPart);
        s_LastFake.QuadPart += (LONGLONG)(delta * Config::Get().game_speed);
    } else {
        s_LastFake.QuadPart += (g_LastRealTimeQPC.QuadPart - s_LastReal.QuadPart);
    }

    s_LastReal = g_LastRealTimeQPC;
    lpPerformanceCount->QuadPart = s_LastFake.QuadPart;
    return TRUE;
}

ULONGLONG WINAPI hk_GetTickCount64() {
    ULONGLONG current_real = o_GetTickCount64();

    static ULONGLONG s_LastRealTick = 0;
    static ULONGLONG s_LastFakeTick = 0;

    std::lock_guard lock(g_TimeMutex);

    if (s_LastRealTick == 0) {
        s_LastRealTick = current_real;
        s_LastFakeTick = current_real;
    }

    if (Config::Get().enable_speedhack) {
        double delta = (double)(current_real - s_LastRealTick);
        s_LastFakeTick += (ULONGLONG)(delta * Config::Get().game_speed);
    } else {
        s_LastFakeTick += (current_real - s_LastRealTick);
    }

    s_LastRealTick = current_real;
    return s_LastFakeTick;
}

bool LoadTextureFromFile(const char* filename, ID3D11Device* device, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    HRESULT coResult = CoInitialize(NULL);

    IWICImagingFactory* iwicFactory = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&iwicFactory));
    
    if (FAILED(hr)) {
        std::cout << "[Error] WIC Factory Create Failed: " << std::hex << hr << std::endl;
        if (coResult == S_OK || coResult == S_FALSE) CoUninitialize();
        return false;
    }

    IWICBitmapDecoder* decoder = nullptr;
    wchar_t wFilename[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, wFilename, MAX_PATH);

    hr = iwicFactory->CreateDecoderFromFilename(wFilename, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr)) {
        std::cout << "[Error] Image File Not Found or Locked: " << filename << std::endl;
        iwicFactory->Release();
        if (coResult == S_OK || coResult == S_FALSE) CoUninitialize();
        return false;
    }

    IWICBitmapFrameDecode* frame = nullptr;
    decoder->GetFrame(0, &frame);

    IWICFormatConverter* converter = nullptr;
    iwicFactory->CreateFormatConverter(&converter);
    
    converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);

    UINT width, height;
    frame->GetSize(&width, &height);
    *out_width = (int)width;
    *out_height = (int)height;
    
    UINT stride = width * 4;
    UINT imageSize = stride * height;
    std::vector<unsigned char> buffer(imageSize);

    converter->CopyPixels(NULL, stride, imageSize, buffer.data());
    
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = buffer.data();
    subResource.SysMemPitch = stride;

    ID3D11Texture2D* pTexture = nullptr;
    device->CreateTexture2D(&desc, &subResource, &pTexture);

    if (pTexture) {
        device->CreateShaderResourceView(pTexture, NULL, out_srv);
        pTexture->Release();
    }
    
    frame->Release();
    converter->Release();
    decoder->Release();
    iwicFactory->Release();

    if (coResult == S_OK || coResult == S_FALSE) CoUninitialize();

    return (*out_srv != nullptr);
}

static float GetProcessCpuUsage() {
    static ULONGLONG lastRun = 0;
    static double cpuUsage = 0.0;
    static FILETIME prevSysKernel, prevSysUser, prevProcKernel, prevProcUser;
    static bool firstRun = true;

    ULONGLONG now = GetTickCount64();
    if (now - lastRun < 500) return (float)cpuUsage;
    lastRun = now;

    FILETIME sysIdle, sysKernel, sysUser;
    FILETIME procCreation, procExit, procKernel, procUser;

    if (!GetSystemTimes(&sysIdle, &sysKernel, &sysUser) ||
        !GetProcessTimes(GetCurrentProcess(), &procCreation, &procExit, &procKernel, &procUser)) {
        return 0.0f;
    }

    if (firstRun) {
        prevSysKernel = sysKernel; prevSysUser = sysUser;
        prevProcKernel = procKernel; prevProcUser = procUser;
        firstRun = false;
        return 0.0f;
    }

    ULARGE_INTEGER ulSysKernel, ulSysUser, ulProcKernel, ulProcUser;
    ULARGE_INTEGER ulPrevSysKernel, ulPrevSysUser, ulPrevProcKernel, ulPrevProcUser;

    ulSysKernel.LowPart = sysKernel.dwLowDateTime; ulSysKernel.HighPart = sysKernel.dwHighDateTime;
    ulSysUser.LowPart = sysUser.dwLowDateTime; ulSysUser.HighPart = sysUser.dwHighDateTime;
    ulProcKernel.LowPart = procKernel.dwLowDateTime; ulProcKernel.HighPart = procKernel.dwHighDateTime;
    ulProcUser.LowPart = procUser.dwLowDateTime; ulProcUser.HighPart = procUser.dwHighDateTime;

    ulPrevSysKernel.LowPart = prevSysKernel.dwLowDateTime; ulPrevSysKernel.HighPart = prevSysKernel.dwHighDateTime;
    ulPrevSysUser.LowPart = prevSysUser.dwLowDateTime; ulPrevSysUser.HighPart = prevSysUser.dwHighDateTime;
    ulPrevProcKernel.LowPart = prevProcKernel.dwLowDateTime; ulPrevProcKernel.HighPart = prevProcKernel.dwHighDateTime;
    ulPrevProcUser.LowPart = prevProcUser.dwLowDateTime; ulPrevProcUser.HighPart = prevProcUser.dwHighDateTime;

    ULONGLONG sysDiff = (ulSysKernel.QuadPart - ulPrevSysKernel.QuadPart) + (ulSysUser.QuadPart - ulPrevSysUser.QuadPart);
    ULONGLONG procDiff = (ulProcKernel.QuadPart - ulPrevProcKernel.QuadPart) + (ulProcUser.QuadPart - ulPrevProcUser.QuadPart);

    if (sysDiff > 0) cpuUsage = (double)procDiff / (double)sysDiff * 100.0;

    prevSysKernel = sysKernel; prevSysUser = sysUser;
    prevProcKernel = procKernel; prevProcUser = procUser;

    return (float)cpuUsage;
}

void* WINAPI hk_PlayerPerspective(void* a1, float a2, void* a3) {
    if (Config::Get().disable_character_fade) {
        a2 = 1.0f; 
    }
    auto orig = (tPlayerPerspective)o_PlayerPerspective.load();
    return orig ? orig(a1, a2, a3) : nullptr;
}

HRESULT __stdcall hk_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!g_dx11Init) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice))) {
            g_pd3dDevice->GetImmediateContext(&g_pd3dContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            g_hGameWindow_ImGui = sd.OutputWindow;
            
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); 
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.IniFilename = nullptr; 
            
            io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
            g_fontBold = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyhbd.ttc", 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
            
            ImGui::StyleColorsDark();
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 10.0f;     
            style.WindowBorderSize = 0.0f;    
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.6f); 
            
            ImGui_ImplWin32_Init(g_hGameWindow_ImGui);
            ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dContext);
            
            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
            pBackBuffer->Release();
            
            std::string dllDir = GetOwnDllDir();
            
            if (g_LogoTexture == nullptr) {
                std::string imagePath = dllDir + "\\logo_banner.png";
                bool loaded = LoadTextureFromFile(imagePath.c_str(), g_pd3dDevice, &g_LogoTexture, &g_LogoWidth, &g_LogoHeight);
            
                if (loaded) {
                    std::cout << "Logo Loaded: " << g_LogoWidth << "x" << g_LogoHeight << std::endl;
                } else {
                    std::cout << "Logo Failed! Path: " << imagePath << std::endl;
                }
            }
            
            g_dx11Init = true;
        }
    }

    if (g_mainRenderTargetView) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            io.MouseDown[0] = true;
        } else {
            io.MouseDown[0] = false;
        }

        ImGui::NewFrame();
        
        {
            ImGuiIO& io = ImGui::GetIO();
            
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, io.DisplaySize.y - 10.0f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
            
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            
            if (ImGui::Begin("##PermanentWatermark", nullptr, 
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | 
                ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | 
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground)) 
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.3f));
                
                ImGui::Text(" ");
                
                ImGui::PopStyleColor();
                ImGui::End();
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }

        static DWORD s_popupStartTime = 0;
        
        if (g_RequestReloadPopup.load()) {
            g_RequestReloadPopup.store(false);
            s_popupStartTime = GetTickCount();
        }
        
        if (s_popupStartTime != 0) {
            if (GetTickCount() - s_popupStartTime > 2000) {
                s_popupStartTime = 0;
            }
            else {
                ImGuiIO& io = ImGui::GetIO();
                ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, 100.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.8f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
                
                if (ImGui::Begin("##ReloadNotify", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing)) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Configuration Reloaded");
                    ImGui::End();
                }
                
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
            }
        }

        if (Config::Get().show_fps_window) {
            auto& cfg = Config::Get();
            ImGui::SetNextWindowPos(ImVec2(cfg.overlay_pos_x, cfg.overlay_pos_y), ImGuiCond_FirstUseEver);
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                                     ImGuiWindowFlags_AlwaysAutoResize | 
                                     ImGuiWindowFlags_NoFocusOnAppearing;
            
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.6f));
            
            if (ImGui::Begin("InfoOverlay", nullptr, flags)) {
                auto& cfg = Config::Get();
                
                static std::vector<float> frameTimes;
                static float low1PercentFps = 0.0f;
                static float calcTimer = 0.0f;
                
                if (io.DeltaTime > 0.0f) {
                    frameTimes.push_back(io.DeltaTime);
                    if (frameTimes.size() > 1000) {
                        frameTimes.erase(frameTimes.begin());
                    }
                }
                
                calcTimer += io.DeltaTime;
                if (calcTimer >= 0.5f) {
                    if (!frameTimes.empty()) {
                        std::vector<float> sortedTimes = frameTimes;
                        std::sort(sortedTimes.begin(), sortedTimes.end());
                        
                        size_t index = sortedTimes.size() * 0.99f;
                        if (index >= sortedTimes.size()) index = sortedTimes.size() - 1;
                        
                        float worstFrameTime = sortedTimes[index];
                        if (worstFrameTime > 0.0f) {
                            low1PercentFps = 1.0f / worstFrameTime;
                        }
                    }
                    calcTimer = 0.0f;
                }
                
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FPS: %.1f | Low 1%%: %.1f", io.Framerate, low1PercentFps);
                
                if (cfg.show_gpu_time) {
                    float frameTime = 1000.0f / (io.Framerate > 0 ? io.Framerate : 1.0f);
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "GPU: %.2f ms", frameTime);
                }
                
                if (cfg.show_cpu_usage) {
                    float cpu = GetProcessCpuUsage();
                    ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "CPU: %.1f %%", cpu);
                }

                if (cfg.show_time) {
                    time_t now = time(0);
                    tm tstruct;
                    localtime_s(&tstruct, &now);
                    ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), "Time: %02d:%02d:%02d", 
                        tstruct.tm_hour, tstruct.tm_min, tstruct.tm_sec);
                }
                
                if (cfg.show_custom_text && !cfg.custom_overlay_text.empty()) {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", cfg.custom_overlay_text.c_str());
                }

                ImVec2 currentPos = ImGui::GetWindowPos();
                
                if (currentPos.x != cfg.overlay_pos_x || currentPos.y != cfg.overlay_pos_y) {
                    if (!ImGui::IsMouseDown(0)) {
                        Config::SaveOverlayPos(currentPos.x, currentPos.y);
                    }
                }

                ImGui::End();
            }
            ImGui::PopStyleColor();
        }
        
        if (Config::Get().show_feature_list) {
            auto& cfg = Config::Get();
            
            struct ActiveFeature {
                std::string name;
                float width;
            };
            std::vector<ActiveFeature> features;
            
            if (g_fontBold) ImGui::PushFont(g_fontBold);
            
            auto AddFeature = [&](const char* name, bool enabled) {
                if (enabled) {
                    features.push_back({ name, ImGui::CalcTextSize(name).x });
                }
            };
            
            AddFeature("No FPS Cap", cfg.enable_fps_override);
            AddFeature("No VSync", cfg.enable_vsync_override);
            AddFeature("Wide FOV", cfg.enable_fov_override);
            AddFeature("Mobile UI", cfg.use_touch_screen);
            AddFeature("No Damage Text", cfg.disable_show_damage_text);
            AddFeature("No Cam Move", cfg.disable_event_camera_move);
            AddFeature("No Fog", cfg.disable_fog);
            AddFeature("No Char Fade", cfg.disable_character_fade);
            AddFeature("Custom Title", cfg.enable_custom_title);
            AddFeature("Craft Redirect", cfg.enable_redirect_craft_override);
            AddFeature("No Team Bar", cfg.enable_remove_team_anim);

            if (g_fontBold) ImGui::PopFont();
            
            if (!features.empty() || g_LogoTexture) {
                
                std::sort(features.begin(), features.end(), [](const ActiveFeature& a, const ActiveFeature& b) {
                    return a.width > b.width;
                });

                ImGuiIO& io = ImGui::GetIO();
                
                ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
                
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                
                if (ImGui::Begin("##HackArrayList", nullptr, 
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | 
                    ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | 
                    ImGuiWindowFlags_NoBackground)) 
                {
                    float time = (float)ImGui::GetTime();
                    float rainbowSpeed = 0.5f;
                    float rainbowScale = 0.05f;

                    if (g_LogoTexture) {
                        float imgW = (float)g_LogoWidth;
                        float imgH = (float)g_LogoHeight;
                        
                        const float MAX_LOGO_WIDTH = 200.0f; 

                        if (imgW > MAX_LOGO_WIDTH) {
                            float scale = MAX_LOGO_WIDTH / imgW;
                            imgW *= scale;
                            imgH *= scale;
                        }
                        
                        float windowWidth = ImGui::GetWindowSize().x;
                        
                        if (windowWidth > imgW) {
                            ImGui::SetCursorPosX(windowWidth - imgW - 5.0f);
                        }
                        
                        ImGui::Image(g_LogoTexture, ImVec2(imgW, imgH));
                        
                        ImGui::Dummy(ImVec2(0, 4.0f)); 
                    }
                    
                    if (g_fontBold) ImGui::PushFont(g_fontBold);

                    for (size_t i = 0; i < features.size(); ++i) {
                        const auto& feat = features[i];

                        float hue = fmodf(time * rainbowSpeed - (float)i * rainbowScale, 1.0f);
                        if (hue < 0.0f) hue += 1.0f;
                        float r, g, b;
                        ImGui::ColorConvertHSVtoRGB(hue, 0.8f, 1.0f, r, g, b);
                        
                        float windowWidth = ImGui::GetWindowSize().x;
                        ImGui::SetCursorPosX(windowWidth - feat.width - 5.0f);
                        
                        ImGui::TextColored(ImVec4(r, g, b, 1.0f), feat.name.c_str());
                    }

                    if (g_fontBold) ImGui::PopFont();

                    ImGui::End();
                }
                ImGui::PopStyleColor();
            }
        }

        ImGui::Render();
        g_pd3dContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return o_Present(pSwapChain, SyncInterval, Flags);
}

void LogOffset(const std::string& name, void* resultAddress, void* instructionAddress = nullptr) {
    if (!Config::Get().dump_offsets || !resultAddress) return;

    HMODULE hMod = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)resultAddress, &hMod)) {
        char modPath[MAX_PATH];
        GetModuleFileNameA(hMod, modPath, sizeof(modPath));
        std::string modName = modPath;
        modName = modName.substr(modName.find_last_of("\\/") + 1);

        uintptr_t base = (uintptr_t)hMod;
        uintptr_t offset = (uintptr_t)resultAddress - base;

        std::string extraInfo = "";
        if (instructionAddress) {
            extraInfo = "  -> [" + GetInstructionInfo((uint8_t*)instructionAddress) + "]";
        }

        std::string filePath = GetOwnDllDir() + "\\offsets.txt";
        std::ofstream file(filePath, std::ios::app);
        if (file.is_open()) {
            file << std::left << std::setw(25) << name 
                 << " = " << modName << "+" << std::hex << std::uppercase << "0x" << offset 
                 << extraInfo << std::dec << std::endl;
        }
    }
}

bool InitDX11Hook() {
    WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), CS_CLASSDC, DefWindowProcA, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "DX11Dummy", NULL };
    
    RegisterClassExA(&wc);
    
    HWND hWnd = CreateWindowA("DX11Dummy", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    IDXGISwapChain* swapChain = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;

    if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 2, D3D11_SDK_VERSION, &sd, &swapChain, &device, &featureLevel, &context))) {
        DestroyWindow(hWnd);
        UnregisterClassA("DX11Dummy", wc.hInstance);
        return false;
    }
    
    void** vTable = *reinterpret_cast<void***>(swapChain);
    void* presentAddr = vTable[8];

    std::cout << "[DX11] Found Present at: " << presentAddr << std::endl;
    if (MH_CreateHook(presentAddr, (void*)hk_Present, (void**)&o_Present) != MH_OK) {
        std::cout << "[DX11] Hook Failed!" << std::endl;
    } else {
        std::cout << "[DX11] Hook Ready." << std::endl;
    }

    swapChain->Release();
    device->Release();
    context->Release();
    DestroyWindow(hWnd);
    
    UnregisterClassA("DX11Dummy", wc.hInstance);
    return true;
}

static HWND g_hGameWindow = NULL;

bool CheckWindowFocused(HWND window) {
    if (!window) return false;
    DWORD foregroundProcessId = 0;
    GetWindowThreadProcessId(window, &foregroundProcessId);
    return foregroundProcessId == GetCurrentProcessId();
}

void UpdateTitleWatermark() {
    if (!Config::Get().enable_custom_title) return;

    if (!g_hGameWindow || !IsWindow(g_hGameWindow)) {
        HWND hForeground = GetForegroundWindow();
        if (hForeground && CheckWindowFocused(hForeground)) {
            g_hGameWindow = hForeground;
        }
    }

    if (!g_hGameWindow) return;

    static ULONGLONG lastTick = 0;
    ULONGLONG currentTick = GetTickCount64();
    if (currentTick - lastTick < 500) return;
    lastTick = currentTick;

    SetWindowTextA(g_hGameWindow, Config::Get().custom_title_text.c_str());
}

void DoCraftLogic() {
    auto findStr = (tFindString)p_FindString.load();
    auto partner = (tCraftPartner)p_CraftPartner.load();
    if (IsValid(findStr) && IsValid(partner)) {
        SafeInvoke([&]
        {
            std::string sPage = XorString::decrypt(EncryptedStrings::SynthesisPage);
            Il2CppString* str = findStr(sPage.c_str());
            if (str) partner(str, nullptr, nullptr, nullptr, nullptr);
        });
    }
}

int32_t WINAPI hk_GetFrameCount() {
    UpdateTitleWatermark();
    auto orig = (tGetFrameCount)o_GetFrameCount.load();
    if (!orig) return 60;
    int32_t ret = 60;
    SafeInvoke([&] { ret = orig(); });
    
    if (ret >= 60) return 60;
    if (ret >= 45) return 45;
    if (ret >= 30) return 30;
    return ret;
}

__int64 WINAPI hk_GameUpdate(__int64 a1, const char* a2) {

    auto orig = (tGameUpdate)o_GameUpdate.load();
    return orig ? orig(a1, a2) : 0;
}

int32_t WINAPI hk_ChangeFov(void* __this, float value) {
    if (!g_GameUpdateInit.load()) g_GameUpdateInit.store(true);
    auto& cfg = Config::Get();
    
    static ULONGLONG lastQPressTime = 0;
    ULONGLONG currentTime = GetTickCount64();
    
    if ((GetAsyncKeyState(0x51) & 0x8000) && (currentTime - lastQPressTime >= 4000)) {
        lastQPressTime = currentTime;
    }
    
    bool isWithinQGracePeriod = (currentTime - lastQPressTime < 4000); 

    if (g_RequestCraft.load()) {
        g_RequestCraft.store(false);
        DoCraftLogic();
    }
    
    if (cfg.enable_vsync_override) {
        auto setSync = (tSetSyncCount)o_SetSyncCount.load();
        if (IsValid(setSync)) SafeInvoke([&]() { setSync(false); });
    }
    
    std::call_once(g_TouchInitOnce, [&]() {
        if (cfg.use_touch_screen) {
            auto sw = (tSwitchInput)p_SwitchInput.load();
            if (IsValid(sw)) SafeInvoke([&]() { sw(nullptr); });
        }
    });

    if (cfg.enable_fps_override) {
        auto setFps = (tSetFrameCount)o_SetFrameCount.load();
        if (IsValid(setFps)) SafeInvoke([&]() { setFps(cfg.selected_fps); });
    }
    
    if (value > 30.0f && cfg.enable_fov_override && !isWithinQGracePeriod) {
        value = cfg.fov_value;
    }

    auto orig = (tChangeFov)o_ChangeFov.load();
    return orig ? orig(__this, value) : 0;
}

void WINAPI hk_SetupQuestBanner(void* __this) {
    auto& cfg = Config::Get();
    auto findStr = (tFindString)p_FindString.load();
    auto findGO = (tFindGameObject)p_FindGameObject.load();
    auto setActive = (tSetActive)p_SetActive.load();

    if (IsValid(findStr) && IsValid(findGO) && IsValid(setActive)) {
        bool hide = false;
        if (cfg.hide_quest_banner) {
            SafeInvoke([&]
            {
                std::string sBanner = XorString::decrypt(EncryptedStrings::QuestBannerPath);
                auto s = findStr(sBanner.c_str());
                if (s) { 
                    auto go = findGO(s); 
                    if (go) { 
                        setActive(go, false); 
                        hide = true; 
                    } 
                }
            });
        }
        if (hide) return;
    }

    auto orig = (tSetupQuestBanner)o_SetupQuestBanner.load();
    if (orig) orig(__this);
}

void WINAPI hk_ShowDamage(void* a, int b, int c, int d, float e, Il2CppString* f, void* g, void* h, int i) {
    if (Config::Get().disable_show_damage_text) return;
    auto orig = (tShowDamage)o_ShowDamage.load();
    if (orig) orig(a, b, c, d, e, f, g, h, i);
}

bool WINAPI hk_EventCamera(void* a, void* b) {
    if (Config::Get().disable_event_camera_move) return true;
    auto orig = (tEventCamera)o_EventCamera.load();
    return orig ? orig(a, b) : true;
}

void WINAPI hk_CraftEntry(void* __this) {
    if (Config::Get().enable_redirect_craft_override) {
        DoCraftLogic();
        return;
    }
    auto orig = (tCraftEntry)o_CraftEntry.load();
    if (orig) orig(__this);
}

void WINAPI hk_OpenTeam() {
    if (Config::Get().enable_remove_team_anim) {
        auto check = (tCheckCanEnter)p_CheckCanEnter.load();
        auto openPage = (tOpenTeamPage)p_OpenTeamPage.load();
        if (IsValid(check) && IsValid(openPage)) {
            bool canEnter = false;
            SafeInvoke([&] { canEnter = check(); });
            if (canEnter) {
                SafeInvoke([&] { openPage(false); });
                return;
            }
        }
    }
    auto orig = (tOpenTeam)o_OpenTeam.load();
    if (orig) orig();
}

__int64 hk_DisplayFog(__int64 a1, __int64 a2) {
    if (Config::Get().disable_fog && a2) {
        
        memset(&g_fogBuf, 0, sizeof(g_fogBuf));
        
        memcpy(g_fogBuf.data, (void*)a2, 64);
        
        g_fogBuf.data[0] = 0;
        
        auto orig = (tDisplayFog)o_DisplayFog.load();
        
        if (orig) return orig(a1, (__int64)g_fogBuf.data);
    }
    
    auto orig = (tDisplayFog)o_DisplayFog.load();
    return orig ? orig(a1, a2) : 0;
}

bool Hooks::Init() {
    if (Config::Get().dump_offsets) {
        std::string filePath = GetOwnDllDir() + "\\offsets.txt";
        std::ofstream file(filePath, std::ios::trunc);
        if (file.is_open()) {
            file << "Feature Offsets Dump" << std::endl;
            file << "====================" << std::endl;
            file << "Generated on module init." << std::endl << std::endl;
        }
    }
    if (MH_Initialize() != MH_OK) return false;
    HOOK_REL("GameUpdate", EncryptedPatterns::GameUpdate, hk_GameUpdate, o_GameUpdate);
    HOOK_REL("GetFrameCount", EncryptedPatterns::GetFrameCount, hk_GetFrameCount, o_GetFrameCount);
    SCAN_REL("SetFrameCount", EncryptedPatterns::SetFrameCount, o_SetFrameCount);
    HOOK_DIR("ChangeFOV", EncryptedPatterns::ChangeFOV, hk_ChangeFov, o_ChangeFov);
    SCAN_DIR("SwitchInput", EncryptedPatterns::SwitchInput, p_SwitchInput);
    HOOK_DIR("QuestBanner", EncryptedPatterns::QuestBanner, hk_SetupQuestBanner, o_SetupQuestBanner);
    SCAN_REL("FindGameObject", EncryptedPatterns::FindGameObject, p_FindGameObject);
    SCAN_REL("SetActive", EncryptedPatterns::SetActive, p_SetActive);
    HOOK_DIR("DamageText", EncryptedPatterns::DamageText, hk_ShowDamage, o_ShowDamage);
    HOOK_DIR("EventCamera", EncryptedPatterns::EventCamera, hk_EventCamera, o_EventCamera);
    SCAN_DIR("FindString", EncryptedPatterns::FindString, p_FindString);
    SCAN_DIR("CraftPartner", EncryptedPatterns::CraftPartner, p_CraftPartner);
    HOOK_DIR("CraftEntry", EncryptedPatterns::CraftEntry, hk_CraftEntry, o_CraftEntry);
    SCAN_DIR("CheckCanEnter", EncryptedPatterns::CheckCanEnter, p_CheckCanEnter);
    SCAN_DIR("OpenTeamPage", EncryptedPatterns::OpenTeamPage, p_OpenTeamPage);
    HOOK_DIR("OpenTeam", EncryptedPatterns::OpenTeam, hk_OpenTeam, o_OpenTeam);
    HOOK_DIR("DisplayFog", EncryptedPatterns::DisplayFog, hk_DisplayFog, o_DisplayFog);
    HOOK_REL("PlayerPerspective", EncryptedPatterns::PlayerPerspective, hk_PlayerPerspective, o_PlayerPerspective);
    SCAN_REL("SetSyncCount", EncryptedPatterns::SetSyncCount, o_SetSyncCount);
    if (Config::Get().enable_dx11_hook) {
        if (!InitDX11Hook()) {
            std::cout << "[FATAL] InitDX11Hook Failed!" << std::endl;
        }
    } else {
        std::cout << "[INFO] DX11 Hook skipped by config." << std::endl;
    }

    {
        HMODULE hKernel32 = GetModuleHandleA("kernel32.dll"); //
        if (hKernel32) {
            void* addrQPC = (void*)GetProcAddress(hKernel32, "QueryPerformanceCounter"); //
            if (addrQPC) {
                std::cout << "[SCAN] QueryPerformanceCounter..." << std::endl; //
                LogOffset("QueryPerformanceCounter", addrQPC, addrQPC);       //
                std::cout << "   -> Found at: " << addrQPC << std::endl;      //
                
                if (MH_CreateHook(addrQPC, &hk_QueryPerformanceCounter, (LPVOID*)&o_QueryPerformanceCounter) == MH_OK) //
                    std::cout << "   -> Hook Ready." << std::endl;            //
            }
            
            void* addrGTC = (void*)GetProcAddress(hKernel32, "GetTickCount64"); //
            if (addrGTC) {
                std::cout << "[SCAN] GetTickCount64..." << std::endl;        //
                LogOffset("GetTickCount64", addrGTC, addrGTC);                //
                std::cout << "   -> Found at: " << addrGTC << std::endl;      //
                
                if (MH_CreateHook(addrGTC, &hk_GetTickCount64, (LPVOID*)&o_GetTickCount64) == MH_OK) //
                    std::cout << "   -> Hook Ready." << std::endl;            //
            }
        }
    }
    
    if (MH_CreateHookApi(L"ws2_32.dll", "send", (void*)hk_send, (void**)&o_send) == MH_OK) {
        std::cout << "[SCAN] Hook send Ready." << std::endl;
    }
    
    if (MH_CreateHookApi(L"ws2_32.dll", "sendto", (void*)hk_sendto, (void**)&o_sendto) == MH_OK) {
        std::cout << "[SCAN] Hook sendto Ready." << std::endl;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        std::cout << "[SCAN] MH_EnableHook Failed!" << std::endl;
        return false;
    }
    return true;
}

void Hooks::Uninit() { 
    if (g_dx11Init) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
        if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
        if (g_pd3dContext) { g_pd3dContext->Release(); g_pd3dContext = nullptr; }
    }
    MH_DisableHook(MH_ALL_HOOKS); 
    MH_Uninitialize(); 
}

bool Hooks::IsGameUpdateInit() { return o_GetFrameCount.load() != nullptr; }
void Hooks::RequestOpenCraft() { g_RequestCraft.store(true); }

void Hooks::TriggerReloadPopup() {
    g_RequestReloadPopup.store(true);
}

void Hooks::InitHSRFps() {
    if (p_HSRFpsAddr.load()) return; 
    
    std::string pat1 = XorString::decrypt(EncryptedPatterns::HSR_FPS_1);
    if (void* addr = Scanner::ScanMainMod(pat1)) {
        if (void* target = Scanner::ResolveRelative(addr, 15, 23)) {
            p_HSRFpsAddr.store(target);
            std::cout << "[HSR] FPS Pattern 1 found: " << target << std::endl;
            return;
        }
    }
    
    std::string pat2 = XorString::decrypt(EncryptedPatterns::HSR_FPS_2);
    if (void* addr = Scanner::ScanMainMod(pat2)) {
        if (void* target = Scanner::ResolveRelative(addr, 11, 19)) {
            p_HSRFpsAddr.store(target);
            std::cout << "[HSR] FPS Pattern 2 found: " << target << std::endl;
            return;
        }
    }
    
    std::string pat3 = XorString::decrypt(EncryptedPatterns::HSR_FPS_3);
    if (void* addr = Scanner::ScanMainMod(pat3)) {
        if (void* target = Scanner::ResolveRelative(addr, 9, 17)) {
            p_HSRFpsAddr.store(target);
            std::cout << "[HSR] FPS Pattern 3 found: " << target << std::endl;
            return;
        }
    }

    std::cout << "[HSR] FPS Pattern NOT found." << std::endl;
}

void Hooks::UpdateHSRFps() {
    void* ptr = p_HSRFpsAddr.load();
    if (ptr && Config::Get().enable_hsr_fps) {
        int32_t* pVal = static_cast<int32_t*>(ptr);
        
        int32_t targetFps = Config::Get().selected_fps; 
        if (targetFps < 60) targetFps = 120;
        
        if (*pVal != targetFps) {
            *pVal = targetFps;
        }
    }
}