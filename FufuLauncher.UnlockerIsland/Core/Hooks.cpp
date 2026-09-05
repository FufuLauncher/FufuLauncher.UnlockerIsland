/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Hooks.h"
#include "../Scanner/Scanner.h"
#include "../Config/Config.h"
#include "Utils.h"
#include "../MinHook/MinHook.h"
#include "SharedState.h"
#include "../Patterns/Patterns.h"
#include "../Automation/Automation.h"
#include "../RainbowDamage/RainbowDamage.h"
#include "../HideUI/HideUI.h"
#include "../Network/Network.h"
#include "../Visual/Visual.h"
#include "../UnderwaterMask/UnderwaterMask.h"
#include "../FreeCamera/FreeCamera.h"
#include "../Camera/Camera.h"
#include "../CameraOffset/CameraOffset.h"
#include "../PaimonFollow/PaimonFollow.h"
#include <iostream>
#include <atomic>
#include <mutex>
#include <string>
#include <d3d11.h>
#include <processthreadsapi.h>
#include <algorithm>
#include <fstream>
#include <sstream>

#pragma comment(lib, "d3d11.lib")

static void TryInitTouchScreen() {
    auto sw = (tSwitchInput)p_SwitchInput.load();
    if (IsValid(sw)) {
        g_TouchScreenInit.store(true);
        __try {
            sw(nullptr);
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
}
#pragma comment(lib, "MinHook/libMinHook.x64.lib")
#pragma comment(lib, "ws2_32.lib")

using tSetCursor = HCURSOR(WINAPI*)(HCURSOR);
static tSetCursor o_SetCursor = nullptr;

static HCURSOR WINAPI hk_SetCursor(HCURSOR cursor) {
    if (cursor && Config::Get().use_system_cursor) {
        // IDC_ARROW resolves to the active Windows "Normal Select" cursor,
        // including a user-selected cursor scheme; no cursor file is fixed.
        static HCURSOR systemNormal = static_cast<HCURSOR>(LoadImageW(
            nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0,
            LR_SHARED | LR_DEFAULTSIZE));
        if (systemNormal) cursor = systemNormal;
    }
    return o_SetCursor ? o_SetCursor(cursor) : nullptr;
}

#define HOOK_REL(name, pat, hookFn, storeOrig) \
    { \
        std::cout << "[SCAN] " << name << "..." << std::endl; \
        void* addr = Scanner::ScanMainMod(pat); \
        if (addr) { \
            void* target = Scanner::ResolveRelative(addr, 1, 5); \
            if (target) { \
            LogOffset(name, target, addr); \
                std::cout << "   -> Found at: 0x" << std::hex << ((long long)target - (long long)GetModuleHandle(nullptr)) << std::endl; \
                if (MH_CreateHook(target, (void*)hookFn, (void**)&storeOrig) == MH_OK) \
                    std::cout << "   -> Hook Ready." << std::endl; \
                else std::cout << "   -> [ERR] MH_CreateHook Failed." << std::endl; \
            } else std::cout << "   -> [ERR] ResolveRelative Failed." << std::endl; \
        } else std::cout << "   -> [ERR] Pattern Not Found." << std::endl; \
    }

#define HOOK_DIR(name, pat, hookFn, storeOrig) \
    { \
        std::cout << "[SCAN] " << name << "..." << std::endl; \
        void* addr = Scanner::ScanMainMod(pat); \
        if (addr) { \
            LogOffset(name, addr, addr); \
            std::cout << "   -> Found at: 0x" << std::hex << ((long long)addr - (long long)GetModuleHandle(nullptr)) << std::endl; \
            if (MH_CreateHook(addr, (void*)hookFn, (void**)&storeOrig) == MH_OK) \
                 std::cout << "   -> Hook Ready." << std::endl; \
            else std::cout << "   -> [ERR] MH_CreateHook Failed." << std::endl; \
        } else std::cout << "   -> [ERR] Pattern Not Found." << std::endl; \
    }

#define SCAN_REL(name, pat, storePtr) \
    { \
        std::cout << "[SCAN] " << name << "..." << std::endl; \
        void* addr = Scanner::ScanMainMod(pat); \
        if (addr) { \
            void* target = Scanner::ResolveRelative(addr, 1, 5); \
            LogOffset(name, target, addr); \
            if (target) { \
            storePtr.store(target); \
            std::cout << "   -> Found at: 0x" << std::hex << ((long long)target - (long long)GetModuleHandle(nullptr)) << std::endl; } \
        } else std::cout << "   -> [ERR] Not Found." << std::endl; \
    }

#define SCAN_DIR(name, pat, storePtr) \
    { \
        std::cout << "[SCAN] " << name << "..." << std::endl; \
        void* addr = Scanner::ScanMainMod(pat); \
        if (addr) { \
            storePtr.store(addr); LogOffset(name, addr, addr); \
            std::cout << "   -> Found at: 0x" << std::hex << ((long long)addr - (long long)GetModuleHandle(nullptr)) << std::endl; } \
        else std::cout << "   -> [ERR] Not Found." << std::endl; \
    }

static uintptr_t ResolveAddress(uintptr_t addr) {
    unsigned char* p = (unsigned char*)addr;
    if (p[0] == 0xE9) {
        int32_t offset = *(int32_t*)(p + 1);
        return addr + 5 + offset;
    }
    return addr;
}

static __int64 __fastcall hk_UpdateInnerTarget(void* pThis, void* a2, double a3) {
    if (Config::Get().enable_low_render_scale) {
        uintptr_t p = (uintptr_t)pThis;
        *(float*)(p + 0x88) = Config::Get().render_scale_value;
        *(__int64*)(p + 0x94) = 0;
        *(__int64*)(p + 0x9C) = 0;
    }
    auto orig = (tUpdateInnerTarget)o_UpdateInnerTarget.load();
    if (orig) return orig(pThis, a2, a3);
    return 0;
}

static void* GetGetActiveAddr() {
    HMODULE hMod = GetModuleHandle(NULL);
    if (!hMod) return nullptr;
    uintptr_t base = (uintptr_t)hMod;
    std::string offsetStr = Offsets::GetActiveOffset;
    uintptr_t offsetVal = 0;
    std::stringstream ss;
    ss << std::hex << offsetStr;
    ss >> offsetVal;
    void* addr = (void*)(base + offsetVal);
    std::cout << "[SCAN] GetActive resolved via encrypted offset: 0x"
              << std::hex << offsetVal << std::dec << std::endl;
    return addr;
}


static void ClockPageOk_SafeLogic(void* pThis, bool& out_handled) {
    out_handled = false;
    auto& cfg = Config::Get();
    auto orig = (tButtonClicked)o_ClockPageOk.load();

    if (cfg.debug_console) {
        std::cout << "[Clock Debug] OK Button Hook Triggered!" << std::endl;
    }

    if (cfg.enable_clock_speedup) {
        out_handled = true;

        if (orig && !IsBadReadPtr((void*)orig, 1)) {
            orig(pThis);
        }

        auto finishFunc = (tButtonClicked)p_ClockPageFinish.load();
        if (finishFunc && !IsBadReadPtr((void*)finishFunc, 1)) {
            if (cfg.debug_console) {
                std::cout << "[Clock Debug] Forcing Finish UI..." << std::endl;
            }
            finishFunc(pThis);
        }

        auto backFunc = (tClockPageBack)p_ClockPageBack.load();
        if (backFunc && !IsBadReadPtr((void*)backFunc, 1)) {
            if (cfg.debug_console) {
                std::cout << "[Clock Debug] Forcing Back UI..." << std::endl;
            }
            backFunc(pThis, nullptr);
            return;
        }

        auto closeBtnFunc = (tButtonClicked)p_ClockPageClose.load();
        if (!closeBtnFunc || IsBadReadPtr((void*)closeBtnFunc, 1)) {
            return;
        }
        if (cfg.debug_console) {
            std::cout << "[Clock Debug] Forcing Close UI..." << std::endl;
        }

        closeBtnFunc(pThis);
    }
}

void WINAPI hk_ClockPageOk(void* pThis) {
    auto orig = (tButtonClicked)o_ClockPageOk.load();

    if (!pThis || IsBadReadPtr(pThis, sizeof(void*))) {
        if (orig && IsValidCodePtr(orig)) {
            __try { orig(pThis); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
        return;
    }

    bool handled = false;

    __try {
        ClockPageOk_SafeLogic(pThis, handled);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        handled = false;
    }

    if (!handled && orig && IsValidCodePtr(orig)) {
        __try {
            orig(pThis);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }
}

int32_t WINAPI hk_GetFrameCount() {
    UpdateTitleWatermark();

    if (g_ShouldShowDialog.load()) {
        g_ShouldShowDialog.store(false);
        std::lock_guard<std::mutex> lock(g_DialogMutex);

        auto fnStringNew = (FnStringNew)p_StringNew.load();
        auto fnShowDialog = (FnShowDialog)p_ShowDialog.load();

        bool dialogSuccess = false;

        if (fnStringNew != nullptr && fnShowDialog != nullptr &&
            !IsBadReadPtr((void*)fnStringNew, 1) &&
            !IsBadReadPtr((void*)fnShowDialog, 1)) {

            SafeInvoke([&] {
                __int64 strMsg = fnStringNew(g_DialogText.c_str());
                __int64 strOk = fnStringNew("\xE7\xA1\xAE\xE8\xAE\xA4");
                __int64 strNo = fnStringNew("\xE5\x8F\x96\xE6\xB6\x88");

                if (strMsg && strOk && strNo) {
                    fnShowDialog(strMsg, strOk, strNo, 0, 0);
                    dialogSuccess = true;
                }
            });
            }

        if (!dialogSuccess) {
            g_StopDialogPolling.store(true);
        }
    }

    auto orig = (tGetFrameCount)o_GetFrameCount.load();
    if (!orig) return 60;
    int32_t ret = 60;
    SafeInvoke([&] { ret = orig(); });

    if (ret >= 60) return 60;
    if (ret >= 45) return 45;
    if (ret >= 30) return 30;
    
    return ret;
}

auto WINAPI hk_GameUpdate(__int64 a1, const char* a2) -> __int64 {
    auto orig = (tGameUpdate)o_GameUpdate.load();
    return orig ? orig(a1, a2) : 0;
}

static bool CheckCanUseShortcut() {
    auto checkEnter = (tCheckCanEnter)p_CheckCanEnter.load();
    if (checkEnter) {
        bool canEnter = false;
        SafeInvoke([&] { canEnter = checkEnter(); });
        return canEnter;
    }
    return true;
}

static bool IsActiveGameObject(const char* name) {
    auto findString = (tFindString)p_FindString.load();
    auto findGameObject = (tFindGameObject)p_FindGameObject.load();
    auto getActive = (tGetActive)p_GetActive.load();
    if (!IsValid(findString) || !IsValid(findGameObject)) return false;

    bool active = false;
    SafeInvoke([&] {
        Il2CppString* objectName = findString(name);
        if (!objectName) return;

        void* gameObject = findGameObject(objectName);
        if (!gameObject) return;

        active = !IsValid(getActive) || getActive(gameObject);
    });
    return active;
}

static bool IsDialogueOrCutsceneActive() {
    static ULONGLONG lastCheck = 0;
    static bool cachedResult = false;

    ULONGLONG now = GetTickCount64();
    if (lastCheck != 0 && now - lastCheck < 1000) return cachedResult;
    lastCheck = now;

    cachedResult = IsActiveGameObject("TalkDialog") ||
        IsActiveGameObject("TalkDialogV1") ||
        IsActiveGameObject("InLevelCutScenePage");
    return cachedResult;
}

int32_t WINAPI hk_ChangeFov(void* __this, float value) {
    if (!g_GameUpdateInit.load()) g_GameUpdateInit.store(true);

    auto& cfg = Config::Get();

    static int frameCounter = 0;
    frameCounter++;

    if (frameCounter >= 100) {
        frameCounter = 0;
        UpdateHideUID();
        // UpdateHideMainUI(); // disabled: duplicated ProfileUIDPath (see HideUI.cpp)
        UpdateOpenMap();
        PaimonFollow::Tick();
    }


    DWORD now = GetTickCount();
    bool canOpenUI = CheckCanUseShortcut();
    bool isFocused = CheckWindowFocused(GetForegroundWindow());

    if (g_RequestCraft.load()) {
        g_RequestCraft.store(false);
        if (cfg.enable_redirect_craft_override && canOpenUI) {
            std::cout << "[Hotkey] Craft function triggered." << std::endl;

            auto findStr = (tFindString)p_FindString.load();
            auto partner = (tCraftPartner)p_CraftPartner.load();

            if (IsValid(findStr) && IsValid(partner)) {
                SafeInvoke([&] {
                    Il2CppString* str = findStr(GameStrings::SynthesisPage);
                    if (str) partner(str, nullptr, nullptr, nullptr, nullptr);
                    });
            }
        }
    }

    if (isFocused && cfg.enable_auto_cook && (GetAsyncKeyState(cfg.auto_cook_key) & 0x8000) && now - g_LastCookTime > 300) {
        if (canOpenUI) {
            g_TrigCook = true;
            g_LastCookTime = now;
            std::cout << "[Hotkey] Auto Cook function triggered." << std::endl;
        }
    }
    if (isFocused && cfg.enable_auto_expedition && (GetAsyncKeyState(cfg.auto_expedition_key) & 0x8000) && now - g_LastExpTime > 300) {
        if (canOpenUI) {
            g_TrigExp = true;
            g_LastExpTime = now;
            std::cout << "[Hotkey] Auto Expedition function triggered." << std::endl;
        }
    }

    if (g_TrigCook)  { g_TrigCook  = false; DoCookingLogic(); }
    if (g_TrigExp)   { g_TrigExp   = false; DoExpeditionLogic(); }

    if (cfg.enable_vsync_override) {
        auto setSync = (tSetSyncCount)o_SetSyncCount.load();
        if (IsValid(setSync)) SafeInvoke([&]() { setSync(false); });
    }

    if (!g_TouchScreenInit.load() && g_GameUpdateInit.load() && cfg.use_touch_screen) {
        TryInitTouchScreen();
    }

    if (cfg.enable_fps_override) {
        auto setFps = (tSetFrameCount)o_SetFrameCount.load();
        if (IsValid(setFps)) SafeInvoke([&]() { setFps(cfg.selected_fps); });
    }

    // Preserve the game's incoming FOV before an override changes it. The
    // existing <= 30 threshold is the plugin's stable aiming-camera signal.
    bool isAimingCamera = value <= 30.0f;
    bool pass_check = !cfg.enable_fov_limit_check || !isAimingCamera;
    if (pass_check && cfg.enable_fov_override) {
        value = cfg.fov_value;
    }

    auto orig = (tChangeFov)o_ChangeFov.load();
    int32_t ret = orig ? orig(__this, value) : 0;
    bool dialogueOrCutsceneActive = cfg.enable_camera_offset ? IsDialogueOrCutsceneActive() : false;
    Camera::Tick();
    bool freeCameraActive = FreeCamera::IsActive();
    if (freeCameraActive) {
        CameraOffset::SuspendImmediately();
        FreeCamera::Tick();
        return ret;
    }
    if (isAimingCamera) CameraOffset::SuspendImmediately();
    CameraOffset::Tick(
        !isAimingCamera && !dialogueOrCutsceneActive,
        false);
    FreeCamera::Tick();
    return ret;
}

bool Hooks::Init() {
    auto StringToAddr = [](const std::string& hexStr) -> uintptr_t {
        if (hexStr.empty()) return 0;
        uintptr_t addr = 0;
        std::stringstream ss;
        ss << std::hex << hexStr;
        ss >> addr;
        return addr;
    };

    char szFileName[MAX_PATH];
    GetModuleFileNameA(NULL, szFileName, MAX_PATH);
    std::string path(szFileName);
    std::transform(path.begin(), path.end(), path.begin(), ::tolower);
    bool isOS = (path.find("genshinimpact.exe") != std::string::npos);
    Offsets::InitOffsets(isOS);

    void* getActiveAddr = nullptr;
    void* activeScan = Scanner::ScanMainMod(Patterns::GetActive);
    if (activeScan) {
        getActiveAddr = Scanner::ResolveRelative(activeScan, 1, 5);
        if (getActiveAddr) std::cout << "[SCAN] GetActive resolved via signature.\n";
    }
    if (!getActiveAddr) {
        getActiveAddr = GetGetActiveAddr();
        if (getActiveAddr) std::cout << "[SCAN] GetActive resolved via offset fallback.\n";
    }

    if (getActiveAddr) {
        p_GetActive.store(getActiveAddr);
        LogOffset("GameObject.get_active", getActiveAddr, getActiveAddr);
    } else {
        std::cout << "[ERR] Failed to resolve GetActive address" << '\n';
    }

    if (Config::Get().dump_offsets) {
        std::string filePath = GetOwnDllDir() + "\\offsets.txt";
        std::ofstream file(filePath, std::ios::trunc);
        if (file.is_open()) {
            file << "Feature Offsets Dump" << '\n';
            file << "====================" << '\n';
            file << "Generated on module init." << '\n' << '\n';
        }
    }

    if (MH_Initialize() != MH_OK) return false;

    HOOK_REL("GetFrameCount", Patterns::GetFrameCount, hk_GetFrameCount, o_GetFrameCount);
    SCAN_REL("SetFrameCount", Patterns::SetFrameCount, o_SetFrameCount);
    HOOK_DIR("ChangeFOV", Patterns::ChangeFOV, hk_ChangeFov, o_ChangeFov);
    {
        void* touchInputAddr = Scanner::ScanMainMod(Patterns::SwitchInputDeviceToTouchScreen);
        if (touchInputAddr) {
            LogOffset("SwitchInputDeviceToTouchScreen", touchInputAddr, touchInputAddr);
            std::cout << "[SCAN] SwitchInputDeviceToTouchScreen resolved via signature.\n";
        } else {
            HMODULE hMod = GetModuleHandle(NULL);
            uintptr_t offsetTouchInput = StringToAddr(Offsets::TouchInputOffset);
            if (hMod && offsetTouchInput) {
                touchInputAddr = (void*)((uintptr_t)hMod + offsetTouchInput);
                LogOffset("SwitchInputDeviceToTouchScreen", touchInputAddr, touchInputAddr);
                std::cout << "[SCAN] SwitchInputDeviceToTouchScreen resolved via offset: 0x"
                          << std::hex << offsetTouchInput << std::dec << '\n';
            } else {
                std::cout << "[ERR] SwitchInputDeviceToTouchScreen offset is missing.\n";
            }
        }
        if (touchInputAddr) {
            p_SwitchInput.store(touchInputAddr);
        }
    }
    HOOK_DIR("QuestBanner", Patterns::QuestBanner, hk_SetupQuestBanner, o_SetupQuestBanner);
    // ProfilePageRefresh (same function as the upstream SetupPlayerProfilePage
    // hook point) is installed below via signature scan + offset fallback.
    SCAN_DIR("FindGameObject", Patterns::FindGameObject, p_FindGameObject);
    {
        HMODULE hMod = GetModuleHandle(NULL);
        uintptr_t offsetSetActive = StringToAddr(Offsets::SetActiveOffset);
        if (hMod && offsetSetActive) {
            void* setActiveAddr = (void*)((uintptr_t)hMod + offsetSetActive);
            LogOffset("SetActive", setActiveAddr, setActiveAddr);
            std::cout << "[SCAN] SetActive via offset: 0x"
                      << std::hex << offsetSetActive << std::dec << '\n';
            if (MH_CreateHook(setActiveAddr, (void*)hk_SetActive, (void**)&o_SetActive) == MH_OK)
                std::cout << "   -> SetActive Hook Ready." << std::endl;
            else
                std::cout << "   -> [ERR] SetActive MH_CreateHook Failed." << std::endl;
        } else {
            std::cout << "   -> [ERR] SetActive offset is missing." << std::endl;
        }
    }
    {
        // Player profile page refresh entry (PJJKFBCHLKF.POGFHKOPOGP).
        // Event source for profile UID hiding: page refresh implies the
        // UID element was (re)shown; the detour hides it right after the
        // original runs. Non-polling, non-short-circuiting.
        // Signature scan first (version-resilient), hardcoded RVA as fallback.
        void* profileRefreshAddr = Scanner::ScanMainMod(Patterns::ProfilePageRefresh);
        if (profileRefreshAddr) {
            LogOffset("ProfilePageRefresh", profileRefreshAddr, profileRefreshAddr);
            std::cout << "[SCAN] ProfilePageRefresh resolved via signature.\n";
        } else {
            HMODULE hMod = GetModuleHandle(NULL);
            uintptr_t offsetProfileRefresh = StringToAddr(Offsets::ProfilePageRefreshOffset);
            if (hMod && offsetProfileRefresh) {
                profileRefreshAddr = (void*)((uintptr_t)hMod + offsetProfileRefresh);
                LogOffset("ProfilePageRefresh", profileRefreshAddr, profileRefreshAddr);
                std::cout << "[SCAN] ProfilePageRefresh via offset fallback: 0x"
                          << std::hex << offsetProfileRefresh << std::dec << '\n';
            }
        }

        if (profileRefreshAddr) {
            if (MH_CreateHook(profileRefreshAddr, (void*)hk_ProfilePageRefresh, (void**)&o_ProfilePageRefresh) == MH_OK)
                std::cout << "   -> ProfilePageRefresh Hook Ready." << std::endl;
            else
                std::cout << "   -> [ERR] ProfilePageRefresh MH_CreateHook Failed." << std::endl;
        } else {
            std::cout << "   -> [ERR] ProfilePageRefresh not found (signature + offset)." << std::endl;
        }
    }
    SCAN_DIR("GetName", Patterns::GetName, p_GetName);
    HOOK_DIR("DamageText", Patterns::DamageText, hk_ShowDamage, o_ShowDamage);
    SCAN_DIR("FindString", Patterns::FindString, p_FindString);
    SCAN_DIR("CraftPartner", Patterns::CraftPartner, p_CraftPartner);
    HOOK_DIR("CraftEntry", Patterns::CraftEntry, hk_CraftEntry, o_CraftEntry);
    SCAN_DIR("CheckCanEnter", Patterns::CheckCanEnter, p_CheckCanEnter);
    SCAN_DIR("OpenTeamPage", Patterns::OpenTeamPage, p_OpenTeamPage);
    HOOK_DIR("OpenTeam", Patterns::OpenTeam, hk_OpenTeam, o_OpenTeam);
    HOOK_DIR("DisplayFog", Patterns::DisplayFog, hk_DisplayFog, o_DisplayFog);
    HOOK_REL("PlayerPerspective", Patterns::PlayerPerspective, hk_PlayerPerspective, o_PlayerPerspective);
    SCAN_REL("SetSyncCount", Patterns::SetSyncCount, o_SetSyncCount);
    SCAN_DIR("CheckCanOpenMap", Patterns::CheckCanOpenMap, p_CheckCanOpenMap);
    SCAN_DIR("StringNew", Patterns::StringNew, p_StringNew);
    SCAN_DIR("ShowDialog", Patterns::ShowDialog, p_ShowDialog);
    SCAN_DIR("AvatarPaimonAppear", Patterns::AvatarPaimonAppear, p_AvatarPaimonAppear);

    void* eventCameraAddr = nullptr;
    uintptr_t offsetEventCam = StringToAddr(Offsets::EventCameraOffset);
    if (offsetEventCam > 0) {
        eventCameraAddr = (void*)((uintptr_t)GetModuleHandle(NULL) + offsetEventCam);
        std::cout << "[SCAN] EventCamera resolved via explicit offset: 0x" << std::hex << offsetEventCam << std::dec << '\n';
    } else {
        void* scanRes = Scanner::ScanMainMod(Patterns::EventCamera);
        if (scanRes) {
            eventCameraAddr = scanRes;
            std::cout << "[SCAN] EventCamera resolved via signature fallback.\n";
        }
    }

    if (eventCameraAddr) {
        LogOffset("EventCamera", eventCameraAddr, eventCameraAddr);
        if (MH_CreateHook(eventCameraAddr, (void*)hk_EventCamera, (void**)&o_EventCamera) == MH_OK) {
            std::cout << "   -> EventCamera Hook Ready.\n";
        } else {
            std::cout << "   -> [ERR] EventCamera Hook Failed.\n";
        }
    } else {
        std::cout << "   -> [ERR] EventCamera not found.\n";
    }

    std::cout << "[SCAN] Initializing Cooking & Expedition via pattern scanning..." << std::endl;

    InitCooking();
    InitExpedition();
    InitExpHandlerPrologueSafe();

    DWORD oldProtect;
    VirtualProtect(p_CheckCanOpenMap.load(), 5, PAGE_EXECUTE_READWRITE, &oldProtect);

    {
        HMODULE hMod = GetModuleHandle(NULL);
        if (hMod) {
            uintptr_t base = (uintptr_t)hMod;

            uintptr_t offsetClockOk = StringToAddr(Offsets::ClockPageOkOffset);
            if (offsetClockOk) {
                void* clockOkAddr = (void*)(base + offsetClockOk);
                LogOffset("ClockPage.Ok", clockOkAddr, clockOkAddr);
                if (MH_CreateHook(clockOkAddr, (void*)hk_ClockPageOk, (void**)&o_ClockPageOk) == MH_OK) {
                    std::cout << "[SCAN] ClockPageOk hooked via offset at: 0x" << std::hex << offsetClockOk << std::dec << '\n';
                } else {
                    std::cout << "[ERR] Failed to hook ClockPageOk via offset.\n";
                }
            } else {
                std::cout << "[ERR] ClockPageOk offset is missing.\n";
            }

            uintptr_t offsetClockClose = StringToAddr(Offsets::ClockPageCloseOffset);
            if (offsetClockClose) {
                void* clockCloseAddr = (void*)(base + offsetClockClose);
                p_ClockPageClose.store(clockCloseAddr);
                LogOffset("ClockPage.Close", clockCloseAddr, clockCloseAddr);
                std::cout << "[SCAN] ClockPageClose resolved via offset at: 0x" << std::hex << offsetClockClose << std::dec << '\n';
            } else {
                std::cout << "[WARN] ClockPageClose offset is missing.\n";
            }

            uintptr_t offsetClockFinish = StringToAddr(Offsets::ClockPageFinishOffset);
            if (offsetClockFinish) {
                void* clockFinishAddr = (void*)(base + offsetClockFinish);
                p_ClockPageFinish.store(clockFinishAddr);
                LogOffset("ClockPage.Finish", clockFinishAddr, clockFinishAddr);
                std::cout << "[SCAN] ClockPageFinish resolved via offset at: 0x" << std::hex << offsetClockFinish << std::dec << '\n';
            } else {
                std::cout << "[WARN] ClockPageFinish offset is missing; falling back to OK+Close.\n";
            }

            uintptr_t offsetClockBack = StringToAddr(Offsets::ClockPageBackOffset);
            if (offsetClockBack) {
                void* clockBackAddr = (void*)(base + offsetClockBack);
                p_ClockPageBack.store(clockBackAddr);
                LogOffset("ClockPage.Back", clockBackAddr, clockBackAddr);
                std::cout << "[SCAN] ClockPageBack resolved via offset at: 0x" << std::hex << offsetClockBack << std::dec << '\n';
            } else {
                std::cout << "[WARN] ClockPageBack offset is missing; falling back to Close path.\n";
            }
        } else {
            std::cout << "[ERR] Critical: GetModuleHandle failed!" << '\n';
        }
    }

    if (Config::Get().enable_rainbow_damage) {
        std::cout << "[SCAN] Hooking Rainbow Damage Colors..." << std::endl;

        uintptr_t base = (uintptr_t)GetModuleHandle(NULL);

        auto ParseOffset = [](const std::string& hexStr) -> uintptr_t {
            uintptr_t val = 0; std::stringstream ss; ss << std::hex << hexStr; ss >> val; return val;
        };

        uintptr_t offA = ParseOffset(Offsets::DamageColorAOffset);
        uintptr_t offB = ParseOffset(Offsets::DamageColorBOffset);
        uintptr_t off1 = ParseOffset(Offsets::DamageColor1Offset);
        uintptr_t off2 = ParseOffset(Offsets::DamageColor2Offset);
        uintptr_t off3 = ParseOffset(Offsets::DamageColor3Offset);
        uintptr_t off4 = ParseOffset(Offsets::DamageColor4Offset);

        if (offA) MH_CreateHook((void*)(base + offA), (void*)RainbowDamageFeature::HookGetColorA, (void**)&RainbowDamageFeature::g_oGetColorA);
        if (offB) MH_CreateHook((void*)(base + offB), (void*)RainbowDamageFeature::HookGetColorB, (void**)&RainbowDamageFeature::g_oGetColorB);
        if (off1) MH_CreateHook((void*)(base + off1), (void*)RainbowDamageFeature::HookGetColor1, (void**)&RainbowDamageFeature::g_oGetColor1);
        if (off2) MH_CreateHook((void*)(base + off2), (void*)RainbowDamageFeature::HookGetColor2, (void**)&RainbowDamageFeature::g_oGetColor2);
        if (off3) MH_CreateHook((void*)(base + off3), (void*)RainbowDamageFeature::HookGetColor3, (void**)&RainbowDamageFeature::g_oGetColor3);
        if (off4) MH_CreateHook((void*)(base + off4), (void*)RainbowDamageFeature::HookGetColor4, (void**)&RainbowDamageFeature::g_oGetColor4);

        CreateThread(nullptr, 0, RainbowDamageFeature::ColorCycleThread, nullptr, 0, nullptr);
        std::cout << "   -> Rainbow Damage Hooks Ready." << std::endl;
    }

    if (MH_CreateHookApi(L"ws2_32.dll", "send", (void*)hk_send, (void**)&o_send) == MH_OK) {
        std::cout << "[SCAN] Hook send Ready." << '\n';
    }

    if (MH_CreateHookApi(L"ws2_32.dll", "sendto", (void*)hk_sendto, (void**)&o_sendto) == MH_OK) {
        std::cout << "[SCAN] Hook sendto Ready." << '\n';
    }

    if (MH_CreateHookApi(L"user32.dll", "SetCursor", (void*)hk_SetCursor, (void**)&o_SetCursor) == MH_OK) {
        std::cout << "[SCAN] Hook SetCursor Ready." << '\n';
    } else {
        std::cout << "[SCAN] Hook SetCursor Failed." << '\n';
    }

    UnderwaterMask::Init();

    Camera::Init();
    CameraOffset::Init();
    FreeCamera::Init();
    PaimonFollow::Init();
    
    if (Config::Get().enable_low_render_scale) {
        uintptr_t offsetUpdateTarget = StringToAddr(Offsets::UpdateInnerTargetOffset);
        if (offsetUpdateTarget) {
            void* updateTargetAddr = (void*)((uintptr_t)GetModuleHandle(NULL) + offsetUpdateTarget);
            LogOffset("UpdateInnerTarget", updateTargetAddr, updateTargetAddr);
            std::cout << "[SCAN] UpdateInnerTarget resolved via offset at: 0x" << std::hex << offsetUpdateTarget << std::dec << std::endl;
            if (MH_CreateHook(updateTargetAddr, (void*)hk_UpdateInnerTarget, (void**)&o_UpdateInnerTarget) == MH_OK) {
                std::cout << "   -> UpdateInnerTarget Hook Ready." << std::endl;
            } else {
                std::cout << "   -> [ERR] UpdateInnerTarget MH_CreateHook Failed." << std::endl;
            }
        } else {
            std::cout << "[ERR] UpdateInnerTarget offset is missing." << std::endl;
        }
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        std::cout << "[SCAN] MH_EnableHook Failed!" << '\n';
        return false;
    }

    return true;
}

bool Hooks::IsGameUpdateInit() { return o_GetFrameCount.load() != nullptr; }
void Hooks::RequestOpenCraft() { g_RequestCraft.store(true); }

void Hooks::TriggerReloadPopup() {
    // Config was just (re)loaded; re-apply profile privacy so a toggle while
    // the profile page is open takes effect immediately.
    ApplyProfilePrivacyState();
    g_RequestReloadPopup.store(true);
}


void Hooks::Uninit() {}
void Hooks::UpdateVisuals() {}
