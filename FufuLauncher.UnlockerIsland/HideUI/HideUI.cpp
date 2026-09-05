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

void UpdateHideMainUI() {
    auto& config = Config::Get();
    if (!config.hide_main_ui) return;

    static float last_check_time = 0.0f;

    auto SetActive = (tSetActive)o_SetActive.load();
    if (!SetActive) return;

    float current_time = (float)clock() / CLOCKS_PER_SEC;
    if (current_time - last_check_time > 2.0f) {
        last_check_time = current_time;

        auto FindString = (tFindString)p_FindString.load();
        auto FindGameObject = (tFindGameObject)p_FindGameObject.load();

        if (FindString && FindGameObject) {
            auto str_obj = FindString(GameStrings::UIDPathMain);
            if (str_obj) {
                void* foundObj = FindGameObject(str_obj);
                if (foundObj) {
                    SetActive(foundObj, false);
                }
            }
        }
    }
}

static bool SetProfileUIDActive(bool active) {
    auto SetActive = (tSetActive)o_SetActive.load();
    if (!SetActive) return false;

    auto FindString = (tFindString)p_FindString.load();
    auto FindGameObject = (tFindGameObject)p_FindGameObject.load();
    if (!FindString || !FindGameObject) return false;

    bool updated = false;
    SafeInvoke([&] {
        auto str_obj = FindString(GameStrings::ProfileUIDPath);
        if (str_obj) {
            void* foundObj = FindGameObject(str_obj);
            if (foundObj) {
                SetActive(foundObj, active);
                updated = true;
            }
        }
    });
    return updated;
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

// Event-driven profile privacy. Called from the SetupPlayerProfilePage hook
// right after the original has opened/set up the page, so the page renders
// normally while the UID / birthday objects are force-hidden below.
void ApplyProfilePrivacyState() {
    const auto& config = Config::Get();
    const bool hideUid = config.hide_profile_uid;
    const bool hideBirthday = config.hide_profile_birthday;
    if (!hideUid && !hideBirthday) return;

    if (hideUid) {
        SetProfileUIDActive(false);
    }
    if (hideBirthday) {
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
