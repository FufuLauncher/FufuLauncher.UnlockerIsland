/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "PaimonFollow.h"

#include "../Config/Config.h"
#include "../Core/SharedState.h"
#include "../Core/Utils.h"
#include "../Patterns/Patterns.h"

#include <iostream>

namespace PaimonFollow
{
    namespace
    {
        constexpr ULONGLONG kResummonCooldownMs = 3000;

        void* g_cachedPaimon = nullptr;
        void* g_cachedDivePaimon = nullptr;
        void* g_cachedBeydPaimon = nullptr;
        bool g_cacheValid = false;
        ULONGLONG g_lastResummonTick = 0;

        constexpr size_t kSingletonLoadOff = 0x2C; // mov rcx,[rip+disp]
        constexpr size_t kSummonGateOff = 0x1F; // new summon path gate
        constexpr size_t kFollowGateOff = 0x8E; // new follow path gate

        bool g_structOk = false;
        bool g_structWarned = false;
        uintptr_t g_gateFlags[2] = {};
        size_t g_gateCount = 0;

        bool BytesMatch(const unsigned char* p, const unsigned char* expect,
                        const unsigned char* mask, size_t len)
        {
            for (size_t i = 0; i < len; ++i)
                if ((p[i] & mask[i]) != expect[i]) return false;
            return true;
        }

        bool StructGatesClosed()
        {
            for (size_t i = 0; i < g_gateCount; ++i)
            {
                __try
                {
                    if (*(const unsigned char*)g_gateFlags[i]) return false;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    return false;
                }
            }
            return true;
        }

        bool VerifyStructLayout()
        {
            auto fn = (const unsigned char*)p_AvatarPaimonAppear.load();
            if (!IsValid(fn)) return false;

            __try
            {
                g_gateCount = 0;

                static const unsigned char kExpect[] = {
                    0x48, 0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00, // mov rcx, [rip+disp]
                    0x80, 0xB9, 0xC7, 0x00, 0x00, 0x00, 0x00, // cmp byte ptr [rcx+0C7h], 0
                };
                static const unsigned char kMask[] = {
                    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                };
                if (!BytesMatch(fn + kSingletonLoadOff, kExpect, kMask, sizeof(kExpect)))
                    return false;

                const size_t kGateOffs[] = {kSummonGateOff, kFollowGateOff};
                for (size_t off : kGateOffs)
                {
                    // cmp cs:byte_x, 0; jnz
                    if (fn[off] != 0x80 || fn[off + 1] != 0x3D || fn[off + 6] != 0x00 ||
                        fn[off + 7] != 0x0F || fn[off + 8] != 0x85)
                        return false;
                    int32_t disp = *(const int32_t*)(fn + off + 2);
                    g_gateFlags[g_gateCount++] = (uintptr_t)(fn + off + 7 + disp);
                }
                return StructGatesClosed();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }
        }

        void InvalidateCache()
        {
            g_cacheValid = false;
            g_cachedPaimon = nullptr;
            g_cachedDivePaimon = nullptr;
            g_cachedBeydPaimon = nullptr;
        }

        void* FindGameObjectByPath(const char* path)
        {
            auto findString = (tFindString)p_FindString.load();
            auto findGameObject = (tFindGameObject)p_FindGameObject.load();
            if (!IsValid(findString) || !IsValid(findGameObject)) return nullptr;

            void* result = nullptr;
            SafeInvoke([&]
            {
                Il2CppString* str = findString(path);
                if (!str) return;
                result = findGameObject(str);
            });
            return IsValid(result) ? result : nullptr;
        }

        bool TryGetActive(void* gameObject, bool& active)
        {
            auto getActive = (tGetActive)p_GetActive.load();
            if (!IsValid(getActive) || !IsValid(gameObject)) return false;

            __try
            {
                active = getActive(gameObject);
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }
        }

        void TrySummonPaimon()
        {
            auto appear = (tAvatarPaimonAppear)p_AvatarPaimonAppear.load();
            if (!IsValid(appear)) return;

            SafeInvoke([&] { appear(nullptr, nullptr, true); });
        }
    }

    void Init()
    {
        if (!IsValid(p_AvatarPaimonAppear.load()))
        {
            std::cout << "   -> [ERR] PaimonFollow disabled: AvatarPaimonAppear signature not found." << std::endl;
            return;
        }

        bool depsReady = IsValid(p_FindString.load()) &&
            IsValid(p_FindGameObject.load()) &&
            IsValid(p_GetActive.load());
        if (!depsReady)
        {
            std::cout << "   -> [ERR] PaimonFollow disabled: dependent engine functions unavailable." << std::endl;
            return;
        }

        g_structOk = VerifyStructLayout();
        if (!g_structOk)
        {
            std::cout << "   -> [ERR] PaimonFollow disabled: struct layout changed." << std::endl;
            return;
        }

        std::cout << "   -> PaimonFollow ready." << std::endl;
    }

    void Tick()
    {
        if (!Config::Get().enable_paimon_follow) return;
        if (!IsValid(p_AvatarPaimonAppear.load())) return;
        if (!g_structOk) return;

        if (!StructGatesClosed())
        {
            if (!g_structWarned)
            {
                g_structWarned = true;
                std::cout << "   -> [WARN] PaimonFollow inactive: game switched to the new struct path." << std::endl;
            }
            return;
        }

        if (!g_cacheValid)
        {
            g_cachedPaimon = FindGameObjectByPath(GameStrings::PaimonPath);
            g_cachedDivePaimon = FindGameObjectByPath(GameStrings::DivePaimonPath);
            g_cachedBeydPaimon = FindGameObjectByPath(GameStrings::BeydPaimonPath);

            if (!g_cachedPaimon || !g_cachedDivePaimon || !g_cachedBeydPaimon)
            {
                return;
            }
            g_cacheValid = true;
        }

        bool anyActive = false;
        bool active = false;
        if (!TryGetActive(g_cachedPaimon, active))
        {
            InvalidateCache();
            return;
        }
        anyActive = anyActive || active;
        if (!TryGetActive(g_cachedDivePaimon, active))
        {
            InvalidateCache();
            return;
        }
        anyActive = anyActive || active;
        if (!TryGetActive(g_cachedBeydPaimon, active))
        {
            InvalidateCache();
            return;
        }
        anyActive = anyActive || active;

        if (anyActive) return;

        ULONGLONG now = GetTickCount64();
        if (g_lastResummonTick != 0 && now - g_lastResummonTick < kResummonCooldownMs)
        {
            return;
        }
        g_lastResummonTick = now;
        TrySummonPaimon();
    }
}
