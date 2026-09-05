/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "HideUI.h"
#include "../Patterns/Patterns.h"
#include "../Config/Config.h"
#include "../Core/Utils.h"
#include <iostream>
#include <ctime>

static HWND g_hGameWindow = NULL;
static const char* g_profile_birthday_resolved_target = nullptr;

static std::atomic g_ShowDamageParamsValid{ true };

bool CheckWindowFocused(HWND window) {
    if (!window) return false;
    DWORD foregroundProcessId = 0;
    GetWindowThreadProcessId(window, &foregroundProcessId);
    return foregroundProcessId == GetCurrentProcessId();
}

void UpdateHideUID() {
    auto& config = Config::Get();
    if (!config.hide_uid) return;

    static float last_check_time = 0.0f;
    float current_time = (float)clock() / CLOCKS_PER_SEC;

    auto SetActive = (tSetActive)o_SetActive.load();
    if (!SetActive) return;

    if (current_time - last_check_time > 2.0f) {
        last_check_time = current_time;

        auto FindString = (tFindString)p_FindString.load();
        auto FindGameObject = (tFindGameObject)p_FindGameObject.load();

        if (FindString && FindGameObject) {
            auto str_obj = FindString(GameStrings::UIDPathWatermark);
            if (str_obj) {
                void* foundObj = FindGameObject(str_obj);
                if (foundObj) {
                    SetActive(foundObj, false);
                }
            }
        }
    }
}

// Disabled: HideMainUI duplicated ProfileUIDPath (same profile UID element)
// and conflicted with the event-driven HideProfileUID hook. Kept for
// reference only.
// void UpdateHideMainUI() {
//     auto& config = Config::Get();
//     if (!config.hide_main_ui) return;
//
//     static float last_check_time = 0.0f;
//
//     auto SetActive = (tSetActive)o_SetActive.load();
//     if (!SetActive) return;
//
//     float current_time = (float)clock() / CLOCKS_PER_SEC;
//     if (current_time - last_check_time > 2.0f) {
//         last_check_time = current_time;
//
//         auto FindString = (tFindString)p_FindString.load();
//         auto FindGameObject = (tFindGameObject)p_FindGameObject.load();
//
//         if (FindString && FindGameObject) {
//             auto str_obj = FindString(GameStrings::UIDPathMain);
//             if (str_obj) {
//                 void* foundObj = FindGameObject(str_obj);
//                 if (foundObj) {
//                     SetActive(foundObj, false);
//                 }
//             }
//         }
//     }
// }

static bool SetProfileBirthdayActive(bool active) {
    auto& config = Config::Get();

    auto SetActive = (tSetActive)o_SetActive.load();
    if (!SetActive) return false;

    auto FindString = (tFindString)p_FindString.load();
    auto FindGameObject = (tFindGameObject)p_FindGameObject.load();
    if (!FindString || !FindGameObject) return false;

    bool updated = false;
    SafeInvoke([&] {
        if (g_profile_birthday_resolved_target) {
            auto str_obj = FindString(g_profile_birthday_resolved_target);
            if (str_obj) {
                void* foundObj = FindGameObject(str_obj);
                if (foundObj) {
                    SetActive(foundObj, active);
                    updated = true;
                }
            }
            return;
        }

        for (const char* target : GameStrings::ProfileBirthdayTargets) {
            auto str_obj = FindString(target);
            if (!str_obj) continue;

            void* foundObj = FindGameObject(str_obj);
            if (foundObj) {
                SetActive(foundObj, active);
                g_profile_birthday_resolved_target = target;
                updated = true;
                if (!active && config.debug_console) {
                    std::cout << "[HideUI] Profile birthday hidden via: "
                              << target << std::endl;
                }
                break;
            }
        }
    });
    return updated;
}

// ===================================================================
// Event-driven profile privacy (Hutao-style, non-polling).
//
// The hooked function is the player profile page refresh entry
// (PJJKFBCHLKF.POGFHKOPOGP, RVA 0x11B39290). It runs every time the
// profile page is opened or refreshed, so it doubles as a precise
// "UID is (re)shown" event source. We call the original first so the
// page fully refreshes (no page short-circuit), then apply privacy
// state once:
//   HideProfileUID = 1       -> hide UID element (cached ptr + SetActive)
//   HideProfileUID = 0       -> restore UID element once (if we hid it)
//   HideProfileBirthday = 1  -> hide birthday element
// ===================================================================

static void* g_cachedProfileUidGo = nullptr;
static bool g_pluginHiddenUid = false;

static bool IsUnityWrapperAlive(void* go) {
    if (!go || (uintptr_t)go <= 0x10000) return false;
    __try {
        // UnityEngine.Object wrapper: native pointer (m_CachedPtr) lives at
        // +0x10 and is cleared by the engine when the object is destroyed.
        void* native = *(void**)((uintptr_t)go + 0x10);
        return native && (uintptr_t)native > 0x10000;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Separate helper: SafeInvoke builds a std::function, which must not share a
// function with __try (C2712). Re-find only when the cached object is gone.
static void* FindProfileUidObject() {
    auto findString = (tFindString)p_FindString.load();
    auto findGO = (tFindGameObject)p_FindGameObject.load();
    if (!IsValid(findString) || !IsValid(findGO)) return nullptr;

    void* found = nullptr;
    SafeInvoke([&] {
        if (auto str_obj = findString(GameStrings::ProfileUIDPath)) {
            found = findGO(str_obj);
        }
    });
    return found;
}

// Separate helper: only pointers here, so __try is allowed (no unwinding).
static void ApplyProfileUidState(void* go, bool active, tSetActive setActive) {
    __try {
        setActive(go, active);
        if (!active && !g_pluginHiddenUid && Config::Get().debug_console) {
            std::cout << "[HideUI] Profile UID hidden on page refresh event." << std::endl;
        }
        g_pluginHiddenUid = !active;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        g_cachedProfileUidGo = nullptr;
    }
}

// Straight state application for the profile UID element:
// config on  -> hide once per call; config off -> restore once if we hid it.
static void SyncProfileUidOnRefresh() {
    const bool shouldHide = Config::Get().hide_profile_uid != 0;

    if (!shouldHide && !g_pluginHiddenUid) return;

    auto setActive = (tSetActive)o_SetActive.load(); // trampoline: calls the real SetActive, bypasses hk_SetActive
    if (!IsValid(setActive)) return;

    if (!g_cachedProfileUidGo || !IsUnityWrapperAlive(g_cachedProfileUidGo)) {
        g_cachedProfileUidGo = FindProfileUidObject();
    }

    void* go = g_cachedProfileUidGo;
    if (!go) return;

    ApplyProfileUidState(go, !shouldHide, setActive);
}

// Event-driven profile privacy. Called from the profile page refresh hook
// right after the original has opened/refreshed the page (page renders
// normally), and from TriggerReloadPopup after a config hot-reload so a
// toggle while the page is open takes effect immediately.
void ApplyProfilePrivacyState() {
    SyncProfileUidOnRefresh();

    if (Config::Get().hide_profile_birthday) {
        SetProfileBirthdayActive(false);
    }
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

void WINAPI hk_SetupQuestBanner(void* __this) {
    auto& cfg = Config::Get();
    auto findStr = (tFindString)p_FindString.load();
    auto findGO = (tFindGameObject)p_FindGameObject.load();
    auto setActive = (tSetActive)o_SetActive.load();

    if (IsValid(findStr) && IsValid(findGO) && IsValid(setActive)) {
        static bool s_is_hidden = false;

        if (cfg.hide_quest_banner) {
            static ULONGLONG last_check_time = 0;
            ULONGLONG current_time = GetTickCount64();

            if (current_time - last_check_time >= 500) {
                last_check_time = current_time;
                bool found = false;

                SafeInvoke([&]
                {
                    auto s = findStr(GameStrings::QuestBannerPath);
                    if (s) {
                        auto go = findGO(s);
                        if (go) {
                            setActive(go, false);
                            found = true;
                        }
                    }
                });

                s_is_hidden = found;
            }

            if (s_is_hidden) return;
        } else {
            s_is_hidden = false;
        }
    }

    auto orig = (tSetupQuestBanner)o_SetupQuestBanner.load();
    if (orig) orig(__this);
}

void WINAPI hk_ShowDamage(void* a, int b, int c, int d, float e, Il2CppString* f, void* g, void* h, int i, char j, float k) {
    auto orig = (tShowDamage)o_ShowDamage.load();

    if (!Config::Get().disable_show_damage_text) {
        if (orig) orig(a, b, c, d, e, f, g, h, i, j, k);
        return;
    }

    if (g_ShowDamageParamsValid.load()) {
        bool abnormal = IsBadReadPtr(a, 4) ||
                        (j != 0 && j != 1) ||
                        !std::isfinite(k) || k < 0.0f;
        if (abnormal) {
            g_ShowDamageParamsValid.store(false);
            std::cout << "[WARN] DamageText params abnormal, feature disabled for safety." << std::endl;
        }
    }

    if (g_ShowDamageParamsValid.load()) return;

    if (orig) orig(a, b, c, d, e, f, g, h, i, j, k);
}

__int64 __fastcall hk_ProfilePageRefresh(void* pThis, __int64 a2) {
    auto orig = (tProfilePageRefresh)o_ProfilePageRefresh.load();
    __int64 ret = orig ? orig(pThis, a2) : 0;

    // Let the page refresh run above, then apply profile privacy state
    // (hide UID/birthday when configured, restore UID once when disabled).
    ApplyProfilePrivacyState();

    return ret;
}
