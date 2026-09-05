/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "Visual.h"
#include "../Patterns/Patterns.h"
#include "../Config/Config.h"
#include "../Core/Utils.h"
#include "../Automation/Automation.h"
#include "../HideUI/HideUI.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <wincodec.h>

#pragma comment(lib, "windowscodecs.lib")

static const char* GetRegName(int index) {
    static const char* regs[] = { "RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15" };
    if (index >= 0 && index < 16) return regs[index];
    return "???";
}

std::string GetOwnDllDir() {
    char path[MAX_PATH];
    HMODULE hm = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&GetOwnDllDir, &hm)) {
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

void UpdateOpenMap() {
    auto cfg = Config::Get();
    if (!p_CheckCanOpenMap.load()) return;

    unsigned char* patchBytes = (unsigned char*)p_CheckCanOpenMap.load();
    static bool s_originalSaved = false;
    if (!s_originalSaved) {
        originalCheckCanOpenMapBytes[0] = patchBytes[0];
        originalCheckCanOpenMapBytes[1] = patchBytes[1];
        originalCheckCanOpenMapBytes[2] = patchBytes[2];
        originalCheckCanOpenMapBytes[3] = patchBytes[3];
        originalCheckCanOpenMapBytes[4] = patchBytes[4];
        s_originalSaved = true;
    }

    static bool s_patched = false;
    if (cfg.enable_redirect_craft_override) {
        if (!s_patched) {
            patchBytes[0] = 0xB8;
            patchBytes[1] = 0x00;
            patchBytes[2] = 0x00;
            patchBytes[3] = 0x00;
            patchBytes[4] = 0x00;
            s_patched = true;
        }
    } else {
        if (s_patched) {
            patchBytes[0] = originalCheckCanOpenMapBytes[0];
            patchBytes[1] = originalCheckCanOpenMapBytes[1];
            patchBytes[2] = originalCheckCanOpenMapBytes[2];
            patchBytes[3] = originalCheckCanOpenMapBytes[3];
            patchBytes[4] = originalCheckCanOpenMapBytes[4];
            s_patched = false;
        }
    }
}

bool LoadTextureFromFile(const char* filename, ID3D11Device* device, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height) {
    HRESULT coResult = CoInitialize(NULL);

    IWICImagingFactory* iwicFactory = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&iwicFactory));

    if (FAILED(hr)) {
        std::cout << "[Error] WIC Factory Create Failed: " << std::hex << hr << '\n';
        if (coResult == S_OK || coResult == S_FALSE) CoUninitialize();
        return false;
    }

    IWICBitmapDecoder* decoder = nullptr;
    wchar_t wFilename[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, wFilename, MAX_PATH);

    hr = iwicFactory->CreateDecoderFromFilename(wFilename, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr)) {
        std::cout << "[Error] Image File Not Found or Locked: " << filename << '\n';
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

float GetProcessCpuUsage() {
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

void LogOffset(const std::string& name, void* resultAddress, void* instructionAddress) {
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
                 << extraInfo << std::dec << '\n';
        }
    }
}

bool WINAPI hk_EventCamera(void* a, void* b) {
    if (Config::Get().disable_event_camera_move) return true;
    auto orig = (tEventCamera)o_EventCamera.load();
    return orig ? orig(a, b) : true;
}

void WINAPI hk_CraftEntry(void* _this) {
    if (Config::Get().enable_redirect_craft_override) {
        auto findStr = (tFindString)p_FindString.load();
        auto partner = (tCraftPartner)p_CraftPartner.load();
        if (IsValid(findStr) && IsValid(partner)) {
            SafeInvoke([&] {
                Il2CppString* str = findStr(GameStrings::SynthesisPage);
                if (str) partner(str, nullptr, nullptr, nullptr, nullptr);
            });
        }
        return;
    }
    auto orig = (tCraftEntry)o_CraftEntry.load();
    if (orig) orig(_this);
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

static Il2CppString* SafeGetName(tGetName getName, void* obj) {
    if (!getName || !obj) return nullptr;
    __try { return getName(obj); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}

void WINAPI hk_SetActive(void* pThis, bool active) {
    tSetActive orig = (tSetActive)o_SetActive.load();
    if (!orig) return;

    const auto& cfg = Config::Get();

    // Only used for hide-grass. Profile UID/birthday are now hidden by the
    // SetupPlayerProfilePage hook instead (see ApplyProfilePrivacyState).
    Il2CppString* name = nullptr;
    if (active && cfg.hide_grass) {
        auto getName = (tGetName)p_GetName.load();
        if (getName) name = SafeGetName(getName, pThis);
    }

    const wchar_t* objectName =
        name && name->chars && name->length > 0 ? name->chars : nullptr;

    if (active && cfg.hide_grass && objectName) {
        if (cfg.hide_grass_indiscriminate) {
            if (wcsstr(objectName, L"Grass") && !wcsstr(objectName, L"Eff") && !wcsstr(objectName, L"Monster")) {
                return;
            }
        } else {
            if (wcsstr(objectName, L"_Grass_")) {
                for (const auto& prefix : GrassPrefix) {
                    if (wcsstr(objectName, prefix.c_str())) {
                        return;
                    }
                }
            }
        }
    }

    orig(pThis, active);
}

static SafeFogBuffer g_fogBuf = { 0 };

auto hk_DisplayFog(__int64 a1, __int64 a2) -> __int64 {
    if (Config::Get().disable_fog && a2) {
        memcpy(g_fogBuf.data, (void*)a2, 404);
        g_fogBuf.data[0] = 0;
        auto orig = (tDisplayFog)o_DisplayFog.load();
        if (orig) return orig(a1, reinterpret_cast<__int64>(g_fogBuf.data));
    }
    auto orig = (tDisplayFog)o_DisplayFog.load();
    return orig ? orig(a1, a2) : 0;
}

