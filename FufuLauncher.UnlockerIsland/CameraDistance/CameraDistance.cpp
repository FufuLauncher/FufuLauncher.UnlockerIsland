/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "CameraDistance.h"
#include "../Config/Config.h"
#include "../FreeCamera/FreeCamera.h"
#include "../MinHook/MinHook.h"
#include "../Patterns/Patterns.h"
#include "../Scanner/Scanner.h"
#include "../Visual/Visual.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>

namespace CameraDistance
{
    namespace
    {
        // CN 7.1: JAEBHHCCJAP (Initialize), not the Unity transform or FOV.
        using Collect = void(__fastcall*)(void*, double, void*, void*, void*, void*);
        using InitializeRadius = void(__fastcall*)(void*, double, void*, void*, void*);
        using IsRadiusMode = bool(__fastcall*)(const void*, int, void*);
        Collect g_Collect = nullptr;
        InitializeRadius g_InitializeRadius = nullptr;
        IsRadiusMode g_IsRadiusMode = nullptr;

        // PipelineCameraGlobalConfig: ordinary follow limits and the native
        // settings interpolation endpoints. Vehicle/special limits are untouched.
        constexpr size_t kMaxOffsets[] = {0x60, 0x64, 0x68, 0x6C, 0xA4, 0xA8, 0xAC};
        constexpr size_t kLimitCount = sizeof(kMaxOffsets) / sizeof(kMaxOffsets[0]);

        struct SavedLimits
        {
            uint8_t* config = nullptr;
            float values[kLimitCount] = {};
            size_t written = 0;
        };

        void RestoreLimits(SavedLimits& saved)
        {
            // Each address belongs to the current synchronous native call.
            // Never retain a managed config pointer across frames/map changes.
            for (size_t i = 0; i < saved.written; ++i)
            {
                *reinterpret_cast<float*>(saved.config + kMaxOffsets[i]) = saved.values[i];
            }
            saved.written = 0;
        }

        bool ApplyLimits(void* self, void* state, SavedLimits& saved)
        {
            const auto& cfg = Config::Get();
            if (!cfg.enable_camera_distance || !std::isfinite(cfg.camera_max_distance) ||
                FreeCamera::IsActive() || IsCameraPageActiveFromEvents() ||
                !self || !state || !g_IsRadiusMode)
                return false;

            __try
            {
                auto* input = static_cast<uint8_t*>(state);
                // PHHOPIKPLKB is passed unboxed (dump offsets minus 0x10).
                // Normal=0; preserve Parallel/Cinema and scripted radius blends.
                if (*reinterpret_cast<int*>(input + 0x13C) != 0 ||
                    *reinterpret_cast<int*>(input + 0x34) != 0 ||
                    !g_IsRadiusMode(input + 0x4C4, 0, nullptr))
                    return false;

                auto* global = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(self) + 0x10);
                if (!global) return false;
                const float minimum = *reinterpret_cast<float*>(global + 0x34);
                if (!std::isfinite(minimum) || minimum <= 0.0f || minimum >= 6.0f) return false;

                // Read and validate the complete snapshot before the first write.
                for (size_t i = 0; i < kLimitCount; ++i)
                {
                    saved.values[i] = *reinterpret_cast<float*>(global + kMaxOffsets[i]);
                    if (!std::isfinite(saved.values[i]) || saved.values[i] < minimum ||
                        saved.values[i] > 100.0f)
                        return false;
                }
                saved.config = global;
                const float maximum = (std::max)(6.0f, cfg.camera_max_distance);
                for (size_t i = 0; i < kLimitCount; ++i)
                {
                    *reinterpret_cast<float*>(global + kMaxOffsets[i]) = maximum;
                    saved.written = i + 1;
                }
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                RestoreLimits(saved);
                return false;
            }
        }

        void __fastcall HookCollect(void* self, double dt, void* state, void* extra,
                                    void* context, void* method)
        {
            SavedLimits saved;
            ApplyLimits(self, state, saved);
            __try
            {
                // Let the game's saved distance ratio choose the ideal radius.
                g_Collect(self, dt, state, extra, context, method);
            }
            __finally
            {
                RestoreLimits(saved);
            }
        }

        void __fastcall HookInitializeRadius(void* self, double dt, void* state,
                                             void* extra, void* method)
        {
            SavedLimits saved;
            ApplyLimits(self, state, saved);
            __try
            {
                // Native Zoom/collision/damping still resolve the final radius.
                g_InitializeRadius(self, dt, state, extra, method);
            }
            __finally
            {
                RestoreLimits(saved);
            }
        }
    }

    void Init()
    {
        auto* collect = Scanner::ScanMainMod(Patterns::CameraDistanceCollect);
        auto* radius = Scanner::ScanMainMod(Patterns::CameraDistanceInitializeRadius);
        auto* mode = Scanner::ScanMainMod(Patterns::CameraDistanceIsRadiusMode);
        // Check the cached ideal radius and the global-config accessor/upper
        // radius writes as well as the prologues before using fixed field offsets.
        if (!collect || !radius || !mode ||
            !Scanner::ScanRange(collect, 0x560, "F2 0F 11 87 10 03 00 00") ||
            !Scanner::ScanRange(radius, 0xDB0,
                                "48 8B 4F 10 48 85 C9 ? ? ? ? ? ? 0F 57 C0 F3 0F 5A C6 F2 0F 58 F8 F2 0F 10 B7 10 03 00 00")
            ||
            !Scanner::ScanRange(radius, 0xDB0, "F2 0F 11 86 78 04 00 00"))
        {
            std::cout << "[WARN] Camera distance layout unsupported; native distance unchanged.\n";
            return;
        }
        g_IsRadiusMode = reinterpret_cast<IsRadiusMode>(mode);
        if (MH_CreateHook(collect, reinterpret_cast<void*>(HookCollect),
                          reinterpret_cast<void**>(&g_Collect)) != MH_OK)
        {
            std::cout << "[WARN] Camera distance Collect hook failed.\n";
            return;
        }
        if (MH_CreateHook(radius, reinterpret_cast<void*>(HookInitializeRadius),
                          reinterpret_cast<void**>(&g_InitializeRadius)) != MH_OK)
        {
            MH_RemoveHook(collect);
            g_Collect = nullptr;
            std::cout << "[WARN] Camera distance radius hook failed; Collect hook removed.\n";
            return;
        }
        // Install while disabled too, so config reload can enable the feature.
        std::cout << "[SCAN] Third-person camera distance hooks ready (experimental).\n";
    }
}
