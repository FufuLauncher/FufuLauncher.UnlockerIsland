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
static std::atomic<bool> g_profile_privacy_config_reload_pending{ false };
static std::atomic<bool> g_profile_privacy_ui_active{ false };
static bool g_profile_uid_retry_pending = false;
static ULONGLONG g_profile_uid_retry_started = 0;
static ULONGLONG g_profile_uid_last_retry = 0;
static int g_profile_uid_retry_attempts = 0;
static const char* g_profile_birthday_resolved_target = nullptr;
static bool g_profile_uid_last_enabled = false;
static bool g_profile_birthday_last_enabled = false;

static constexpr ULONGLONG PROFILE_UID_RETRY_WINDOW_MS = 1500;
static constexpr ULONGLONG PROFILE_UID_RETRY_INTERVAL_MS = 8;
static constexpr int PROFILE_UID_MAX_RETRY_ATTEMPTS = 12;

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
// and conflicted with the new event-driven HideProfileUID hook. Kept for
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

// Disabled: redundant with the event-driven profile page refresh hook
// (hk_ProfilePageRefresh) + the hk_SetActive name blocker. Kept for
// reference only.
// static bool SetProfileUIDActive(bool active) {
//     auto SetActive = (tSetActive)o_SetActive.load();
//     if (!SetActive) return false;
// 
//     auto FindString = (tFindString)p_FindString.load();
//     auto FindGameObject = (tFindGameObject)p_FindGameObject.load();
//     if (!FindString || !FindGameObject) return false;
// 
//     bool updated = false;
//     SafeInvoke([&] {
//         auto str_obj = FindString(GameStrings::ProfileUIDPath);
//         if (str_obj) {
//             void* foundObj = FindGameObject(str_obj);
//             if (foundObj) {
//                 SetActive(foundObj, active);
//                 updated = true;
//             }
//         }
//     });
//     return updated;
// }

bool UpdateHideProfileUID() {
    // Old path disabled (see SetProfileUIDActive above); always report
    // success so the retry/reload machinery stays dormant. UID hiding and
    // restoring are handled by the profile page refresh event hook.
    return true;
}

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

void UpdateHideProfileBirthday() {
    auto& config = Config::Get();
    if (!config.hide_profile_birthday) return;
    SetProfileBirthdayActive(false);
}

void UpdateProfilePrivacyUI() {
    auto& config = Config::Get();
    if (!config.hide_profile_uid && !config.hide_profile_birthday) return;

    if (UpdateHideProfileUID()) g_profile_uid_retry_pending = false;
    UpdateHideProfileBirthday();
}

bool IsProfilePrivacyUIActive() {
    return g_profile_privacy_ui_active.load(std::memory_order_relaxed);
}

static bool FindActiveProfilePage() {
    auto FindString = (tFindString)p_FindString.load();
    auto FindGameObject = (tFindGameObject)p_FindGameObject.load();
    auto GetActive = (tGetActive)p_GetActive.load();
    if (!FindString || !FindGameObject) return false;

    bool active = false;
    SafeInvoke([&] {
        auto str_obj = FindString(GameStrings::ProfileLayerPath);
        if (!str_obj) return;

        void* foundObj = FindGameObject(str_obj);
        if (!foundObj) return;

        active = !GetActive || GetActive(foundObj);
    });
    return active;
}

static void ResetProfileUIDRetry() {
    g_profile_uid_retry_pending = false;
    g_profile_uid_retry_started = 0;
    g_profile_uid_last_retry = 0;
    g_profile_uid_retry_attempts = 0;
}

void BeginProfilePrivacyUI() {
    auto& config = Config::Get();

    g_profile_privacy_ui_active.store(true, std::memory_order_relaxed);
    ResetProfileUIDRetry();
    g_profile_uid_retry_started = GetTickCount64();

    if (config.hide_profile_uid) {
        g_profile_uid_retry_pending = !UpdateHideProfileUID();
    }
    UpdateHideProfileBirthday();
    g_profile_uid_last_enabled = config.hide_profile_uid;
    g_profile_birthday_last_enabled = config.hide_profile_birthday;
}

void EndProfilePrivacyUI() {
    g_profile_privacy_ui_active.store(false, std::memory_order_relaxed);
    ResetProfileUIDRetry();
}

void NotifyProfileUIDBlocked() {
    g_profile_uid_retry_pending = false;
}

void NotifyProfilePrivacyConfigReload() {
    g_profile_privacy_config_reload_pending.store(true, std::memory_order_release);
}

static void ApplyPendingProfilePrivacyConfigReload() {
    if (!g_profile_privacy_config_reload_pending.exchange(
            false, std::memory_order_acq_rel)) {
        return;
    }

    ResetProfileUIDRetry();
    bool profilePageActive = IsProfilePrivacyUIActive();
    if (!profilePageActive) {
        profilePageActive = FindActiveProfilePage();
        g_profile_privacy_ui_active.store(profilePageActive, std::memory_order_relaxed);
    }

    auto& config = Config::Get();
    if (!profilePageActive) {
        g_profile_uid_last_enabled = config.hide_profile_uid;
        g_profile_birthday_last_enabled = config.hide_profile_birthday;
        return;
    }

    if (config.hide_profile_uid || config.hide_profile_birthday) {
        g_ProfilePrivacyRuntimeReady.store(true, std::memory_order_relaxed);
    }

    if (config.hide_profile_uid) {
        g_profile_uid_retry_started = GetTickCount64();
        g_profile_uid_retry_pending = !UpdateHideProfileUID();
    } else if (g_profile_uid_last_enabled) {
        // SetProfileUIDActive(true); // disabled: restore happens on the next
        // profile page refresh event (hk_ProfilePageRefresh).
    }

    if (config.hide_profile_birthday) {
        SetProfileBirthdayActive(false);
    } else if (g_profile_birthday_last_enabled) {
        SetProfileBirthdayActive(true);
    }

    g_profile_uid_last_enabled = config.hide_profile_uid;
    g_profile_birthday_last_enabled = config.hide_profile_birthday;
}

void UpdatePendingProfilePrivacyUI() {
    ApplyPendingProfilePrivacyConfigReload();
    if (!g_profile_uid_retry_pending) return;

    auto& config = Config::Get();
    if (!config.hide_profile_uid) {
        EndProfilePrivacyUI();
        return;
    }

    ULONGLONG current_time = GetTickCount64();
    if (g_profile_uid_retry_attempts >= PROFILE_UID_MAX_RETRY_ATTEMPTS ||
        current_time - g_profile_uid_retry_started > PROFILE_UID_RETRY_WINDOW_MS) {
        g_profile_uid_retry_pending = false;
        if (config.debug_console) {
            std::cout << "[HideUI] Profile UID retry window expired." << std::endl;
        }
        return;
    }

    if (g_profile_uid_last_retry != 0 &&
        current_time - g_profile_uid_last_retry < PROFILE_UID_RETRY_INTERVAL_MS) {
        return;
    }

    g_profile_uid_last_retry = current_time;
    ++g_profile_uid_retry_attempts;
    if (UpdateHideProfileUID()) {
        g_profile_uid_retry_pending = false;
        if (config.debug_console) {
            std::cout << "[HideUI] Profile UID hidden after "
                      << g_profile_uid_retry_attempts << " bounded retries."
                      << std::endl;
        }
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

// ===================================================================
// Event-driven profile UID hiding (Hutao-style, non-polling).
//
// The hooked function is the player profile page refresh entry
// (PJJKFBCHLKF.POGFHKOPOGP, RVA 0x11B39290). It runs every time the
// profile page is opened or refreshed, so it doubles as a precise
// "UID is (re)shown" event source. We call the original first so the
// page fully refreshes (no page short-circuit), then apply the UID
// state once via a cached GameObject pointer + direct SetActive:
//   config HideProfileUID = 1 -> SetActive(false) (hide)
//   config HideProfileUID = 0 -> SetActive(true)  (restore, once)
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

// Runs on every profile page refresh event (after the original).
// Straight state application: config on  -> hide UID once per event;
// config off -> if we had hidden it, restore (SetActive true) once.
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

__int64 __fastcall hk_ProfilePageRefresh(void* pThis, __int64 a2) {
    auto orig = (tProfilePageRefresh)o_ProfilePageRefresh.load();
    __int64 ret = orig ? orig(pThis, a2) : 0;

    // Let the page refresh run above, then apply the profile UID state
    // once: hide when configured, restore when disabled.
    SyncProfileUidOnRefresh();

    return ret;
}
