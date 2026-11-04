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
        using namespace Offsets::CameraTweaks;
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
                    static_cast<uint8_t*>(*g_InputRootSlot) + (Offsets::IsOS ? InputManagerSlotOS : InputManagerSlot));
                return manager ? *reinterpret_cast<int*>(manager +
                    (Offsets::IsOS ? InputModeFieldOS : InputModeField)) : 0;
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
                auto* velocity = reinterpret_cast<float*>(static_cast<uint8_t*>(state) + AngularVelocity);
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
                const auto* deviceField = Offsets::IsOS
                    ? Patterns::CameraInputManagerFieldOS : Patterns::CameraInputManagerField;
                const auto* modeField = Offsets::IsOS
                    ? Patterns::CameraInputModeFieldOS : Patterns::CameraInputModeField;
                return memcmp(follow + FollowVelocityReset, Patterns::CameraVelocityReset,
                              sizeof(Patterns::CameraVelocityReset)) == 0 &&
                    follow[FollowDecayCall] == 0xE8 && Relative(follow + FollowDecayCall, 1, 5) == decay &&
                    memcmp(decay + DecayVelocityRead, Patterns::CameraVelocityRead,
                           sizeof(Patterns::CameraVelocityRead)) == 0 &&
                    memcmp(decay + DecayVelocityWrite, Patterns::CameraVelocityWrite,
                           sizeof(Patterns::CameraVelocityWrite)) == 0 &&
                    memcmp(follow + FollowHotfixCompare, Patterns::RipRelativeByteCompare,
                           sizeof(Patterns::RipRelativeByteCompare)) == 0 &&
                    memcmp(decay + DecayHotfixCompare, Patterns::RipRelativeByteCompare,
                           sizeof(Patterns::RipRelativeByteCompare)) == 0 &&
                    memcmp(device, deviceField, sizeof(Patterns::CameraInputManagerField)) == 0 &&
                    memcmp(device + InputModeCompare, modeField, sizeof(Patterns::CameraInputModeField)) == 0 &&
                    memcmp(device - InputRootLoadDistance, Patterns::RipRelativeLoad,
                           sizeof(Patterns::RipRelativeLoad)) == 0 &&
                    memcmp(device - InputRootJumpDistance, Patterns::NearNotEqualJump,
                           sizeof(Patterns::NearNotEqualJump)) == 0;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        }

        void __fastcall HookBlenderTick(void* self, float deltaTime)
        {
            if (self && deltaTime >= 0.0f && Config::Get().disable_camera_blend)
            {
                __try
                {
                    auto* state = static_cast<uint8_t*>(self);
                    const float duration = *reinterpret_cast<float*>(state +
                        (Offsets::IsOS ? BlenderDurationOS : BlenderDuration));
                    if (std::isfinite(duration) && duration > 0.0f)
                    {
                        *reinterpret_cast<float*>(state + (Offsets::IsOS ? BlenderElapsedOS : BlenderElapsed)) = duration;
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
        using namespace Offsets::CameraTweaks;
        auto* follow = static_cast<uint8_t*>(FindUniqueCode(Patterns::CameraFollowTick));
        auto* decay = static_cast<uint8_t*>(FindUniqueCode(Patterns::CameraAngularDecay));
        auto* device = static_cast<uint8_t*>(FindUniqueCode(
            Offsets::IsOS ? Patterns::CameraInputDeviceOS : Patterns::CameraInputDevice, false));
        if (ValidateInputPath(follow, decay, device))
        {
            g_InputRootSlot = reinterpret_cast<void**>(Relative(device - InputRootLoadDistance, 3, 7));
            g_FollowHotfix = Relative(follow + FollowHotfixCompare, 2, 7);
            g_DecayHotfix = Relative(decay + DecayHotfixCompare, 2, 7);
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

        // Install while disabled too: configuration reload must be able to enable it.
        {
            auto* code = static_cast<uint8_t*>(Scanner::ScanMainMod(Patterns::CameraStateBlenderTick));
            const auto* timerCode = Offsets::IsOS ? Patterns::CameraBlendTimersOS : Patterns::CameraBlendTimers;
            const auto timerSize = Offsets::IsOS
                ? sizeof(Patterns::CameraBlendTimersOS) : sizeof(Patterns::CameraBlendTimers);
            if (code && !IsBadReadPtr(code, BlenderTimerUpdate + timerSize) &&
                memcmp(code + BlenderTimerUpdate, timerCode, timerSize) == 0 &&
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
