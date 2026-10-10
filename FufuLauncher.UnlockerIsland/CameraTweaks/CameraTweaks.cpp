/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "CameraTweaks.h"

#include "../Config/Config.h"
#include "../MinHook/MinHook.h"
#include "../Patterns/Patterns.h"
#include "../Scanner/Scanner.h"

#include <Windows.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace CameraTweaks
{
    namespace
    {
        using FollowTick = void (__fastcall*)(void*, double, void*, void*);
        using AngularDecay = void (__fastcall*)(double, void*, double, double);
        using BlenderTick = void (__fastcall*)(void*, float);
        FollowTick g_OriginalFollowTick = nullptr;
        AngularDecay g_OriginalAngularDecay = nullptr;
        BlenderTick g_OriginalBlenderTick = nullptr;
        void** g_InputRootSlot = nullptr;
        const uint8_t* g_FollowHotfix = nullptr;
        const uint8_t* g_DecayHotfix = nullptr;
        thread_local void* g_FollowState = nullptr;
        thread_local bool g_DirectMouseInput = false;

        uint8_t* Relative(uint8_t* instruction, size_t displacement, size_t length)
        {
            int32_t delta = 0;
            memcpy(&delta, instruction + displacement, sizeof(delta));
            return instruction + length + delta;
        }

        void* FindUniqueCode(const char* signature, bool functionEntry = true)
        {
            auto* base = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
            auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
            auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
            auto* sections = IMAGE_FIRST_SECTION(nt);
            void* match = nullptr;
            for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i)
            {
                const auto& section = sections[i];
                if (!(section.Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
                auto* begin = base + section.VirtualAddress;
                auto* end = begin + section.Misc.VirtualSize;
                auto* found = static_cast<uint8_t*>(Scanner::ScanRange(begin, end - begin, signature));
                if (!found) continue;
                if (match || Scanner::ScanRange(found + 1, end - (found + 1), signature)) return nullptr;
                match = found;
            }
            if (match && functionEntry)
            {
                DWORD64 imageBase = 0;
                auto* entry = RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(match), &imageBase, nullptr);
                if (!entry || imageBase != reinterpret_cast<DWORD64>(base) ||
                    imageBase + entry->BeginAddress != reinterpret_cast<DWORD64>(match))
                    return nullptr;
            }
            return match;
        }

        int InputMode()
        {
            __try
            {
                if (!g_InputRootSlot || !*g_InputRootSlot) return 0;
                auto* manager = *reinterpret_cast<uint8_t**>(
                    static_cast<uint8_t*>(*g_InputRootSlot) + 0x9CA0);
                return manager ? *reinterpret_cast<int*>(manager + 0x188) : 0;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
        }

        bool NativeMousePath(double deltaTime)
        {
            __try
            {
                return std::isfinite(deltaTime) && deltaTime > 0.0 && deltaTime <= 1.0 &&
                    g_FollowHotfix && !*g_FollowHotfix && g_DecayHotfix && !*g_DecayHotfix &&
                    InputMode() == 2;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        }

        void ClearAngularVelocity(void* state)
        {
            __try
            {
                auto* velocity = reinterpret_cast<float*>(static_cast<uint8_t*>(state) + 0x51C);
                if (!std::isfinite(velocity[0]) || !std::isfinite(velocity[1])) return;
                const uint64_t zero = 0;
                memcpy(velocity, &zero, sizeof(zero));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }

        void __fastcall HookAngularDecay(double deltaTime, void* state, double coefficient, double accuracy)
        {
            g_OriginalAngularDecay(deltaTime, state, coefficient, accuracy);
            if (state && state == g_FollowState && g_DirectMouseInput && Config::Get().disable_camera_smooth)
            {
                ClearAngularVelocity(state);
            }
        }

        struct FollowScope
        {
            void* previousState = g_FollowState;
            bool previousDirect = g_DirectMouseInput;

            FollowScope(void* state, double deltaTime)
            {
                g_FollowState = state;
                g_DirectMouseInput = NativeMousePath(deltaTime);
            }

            ~FollowScope()
            {
                g_FollowState = previousState;
                g_DirectMouseInput = previousDirect;
            }
        };

        void __fastcall HookFollowTick(void* self, double deltaTime, void* state, void* extra)
        {
            FollowScope scope(state, deltaTime);
            g_OriginalFollowTick(self, deltaTime, state, extra);
        }

        bool ValidateInputPath(uint8_t* follow, uint8_t* decay, uint8_t* device)
        {
            __try
            {
                if (!follow || !decay || !device) return false;
                const uint8_t reset[] = {0x48, 0xC7, 0x86, 0x1C, 0x05, 0, 0, 0, 0, 0, 0};
                const uint8_t read[] = {0x4C, 0x8B, 0x86, 0x1C, 0x05, 0, 0};
                const uint8_t write[] = {0x48, 0x89, 0x86, 0x1C, 0x05, 0, 0};
                const uint8_t deviceField[] = {0x48, 0x8B, 0x80, 0xA0, 0x9C, 0, 0};
                const uint8_t modeField[] = {0x83, 0xB8, 0x88, 0x01, 0, 0, 0x03};
                return memcmp(follow + 0x68, reset, sizeof(reset)) == 0 &&
                    follow[0x4AF] == 0xE8 && Relative(follow + 0x4AF, 1, 5) == decay &&
                    memcmp(decay + 0x3A, read, sizeof(read)) == 0 &&
                    memcmp(decay + 0xC9, write, sizeof(write)) == 0 &&
                    follow[0x2B] == 0x80 && follow[0x2C] == 0x3D &&
                    decay[0x2D] == 0x80 && decay[0x2E] == 0x3D &&
                    memcmp(device, deviceField, sizeof(deviceField)) == 0 &&
                    memcmp(device + 0x10, modeField, sizeof(modeField)) == 0 &&
                    device[-0xD] == 0x48 && device[-0xC] == 0x8B && device[-0xB] == 0x05 &&
                    device[-6] == 0x0F && device[-5] == 0x85;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        }

        void __fastcall HookBlenderTick(void* self, float deltaTime)
        {
            if (self && deltaTime >= 0.0f && Config::Get().disable_camera_blend)
            {
                __try
                {
                    auto* state = static_cast<float*>(self);
                    const float duration = state[24];
                    if (std::isfinite(duration) && duration > 0.0f)
                    {
                        state[25] = duration;
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                }
            }
            if (g_OriginalBlenderTick) g_OriginalBlenderTick(self, deltaTime);
        }
    }

    void Init()
    {
        const auto& cfg = Config::Get();
        auto* follow = static_cast<uint8_t*>(FindUniqueCode(Patterns::CameraFollowTick));
        auto* decay = static_cast<uint8_t*>(FindUniqueCode(Patterns::CameraAngularDecay));
        auto* device = static_cast<uint8_t*>(FindUniqueCode(Patterns::CameraInputDevice, false));
        if (ValidateInputPath(follow, decay, device))
        {
            g_InputRootSlot = reinterpret_cast<void**>(Relative(device - 0xD, 3, 7));
            g_FollowHotfix = Relative(follow + 0x2B, 2, 7);
            g_DecayHotfix = Relative(decay + 0x2D, 2, 7);
            const auto decayStatus = MH_CreateHook(decay, reinterpret_cast<void*>(HookAngularDecay),
                                                   reinterpret_cast<void**>(&g_OriginalAngularDecay));
            const auto followStatus = decayStatus == MH_OK
                                          ? MH_CreateHook(follow, reinterpret_cast<void*>(HookFollowTick),
                                                          reinterpret_cast<void**>(&g_OriginalFollowTick))
                                          : MH_UNKNOWN;
            if (decayStatus == MH_OK && followStatus == MH_OK)
            {
                std::cout << "[SCAN] Camera Follow Tick and Angular Decay Hooks Ready.\n";
            }
            else
            {
                if (decayStatus == MH_OK) MH_RemoveHook(decay);
                g_OriginalAngularDecay = nullptr;
                g_OriginalFollowTick = nullptr;
                std::cout << "[WARN] Camera input hooks unavailable; smoothing unchanged.\n";
            }
        }
        else
        {
            std::cout << "[WARN] Camera input path unavailable or layout unsupported; smoothing unchanged.\n";
        }

        if (cfg.disable_camera_blend)
        {
            auto* code = static_cast<uint8_t*>(Scanner::ScanMainMod(Patterns::CameraStateBlenderTick));
            const uint8_t timers[] = {
                0xF3, 0x0F, 0x10, 0x46, 0x60, 0xF3, 0x0F, 0x10, 0x76, 0x64,
                0x0F, 0x28, 0xCE, 0xF3, 0x41, 0x0F, 0x58, 0xC8,
                0xF3, 0x0F, 0x11, 0x4E, 0x64
            };
            if (code && !IsBadReadPtr(code, 0x4F + sizeof(timers)) &&
                memcmp(code + 0x4F, timers, sizeof(timers)) == 0 &&
                MH_CreateHook(code, reinterpret_cast<void*>(HookBlenderTick),
                              reinterpret_cast<void**>(&g_OriginalBlenderTick)) == MH_OK)
            {
                std::cout << "[SCAN] CameraStateBlender Tick Hook Ready.\n";
            }
            else
            {
                std::cout << "[WARN] CameraStateBlender unavailable or layout unsupported; state blending unchanged.\n";
            }
        }
    }
}
