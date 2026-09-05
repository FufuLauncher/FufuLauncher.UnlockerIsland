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

static void* g_profile_uid_cached = nullptr;
static bool g_profile_uid_hidden_by_plugin = false;
static void* g_profile_bday_cached = nullptr;
static bool g_profile_bday_hidden_by_plugin = false;

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
        void* go = nullptr;
        auto str_obj = FindString(GameStrings::ProfileUIDPath);
        if (str_obj) go = FindGameObject(str_obj);
        if (!go) go = g_profile_uid_cached; // inactive objects are invisible to Find
        if (go) {
            SetActive(go, active);
            g_profile_uid_cached = go;
            updated = true;
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
        void* go = nullptr;
        if (g_profile_birthday_resolved_target) {
            auto str_obj = FindString(g_profile_birthday_resolved_target);
            if (str_obj) go = FindGameObject(str_obj);
            if (!go) go = g_profile_bday_cached;
            if (go) {
                SetActive(go, active);
                g_profile_bday_cached = go;
                updated = true;
            }
            return;
        }

        for (const char* target : GameStrings::ProfileBirthdayTargets) {
            auto str_obj = FindString(target);
            if (!str_obj) continue;

            void* foundObj = FindGameObject(str_obj);
            if (foundObj) {
                SetActive(foundObj, active);
                g_profile_bday_cached = foundObj;
                g_profile_birthday_resolved_target = target;
                updated = true;
                if (!active && config.debug_console) {
                    std::cout << "[HideUI] Profile birthday hidden via: "
                              << target << std::endl;
                }
                break;
            }
        }

        if (!updated && g_profile_bday_cached) {
            SetActive(g_profile_bday_cached, active);
            updated = true;
        }
    });
    return updated;
}

void ApplyProfilePrivacyState() {
    const auto& config = Config::Get();
    const bool hideUid = config.hide_profile_uid;
    const bool hideBirthday = config.hide_profile_birthday;

    if (hideUid) {
        if (SetProfileUIDActive(false)) g_profile_uid_hidden_by_plugin = true;
    } else if (g_profile_uid_hidden_by_plugin) {
        if (SetProfileUIDActive(true)) g_profile_uid_hidden_by_plugin = false;
    }

    if (hideBirthday) {
        if (SetProfileBirthdayActive(false)) g_profile_bday_hidden_by_plugin = true;
    } else if (g_profile_bday_hidden_by_plugin) {
        if (SetProfileBirthdayActive(true)) g_profile_bday_hidden_by_plugin = false;
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
