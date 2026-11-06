/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "CameraTweaks.h"

#include "../Config/Config.h"
#include "../Core/Compatibility.h"
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
        using BlenderTick = void (__fastcall*)(void*, float);
        BlenderTick g_OriginalBlenderTick = nullptr;
        using ViewInput = void(__fastcall*)(void*, uint64_t, bool, bool, void*);
        ViewInput g_OriginalViewInput = nullptr;
        void** g_InputRootSlot = nullptr;
        using ViewDispatch = void(__fastcall*)(void*, uint64_t, void*);
        using ViewSensitivity = float(__fastcall*)(void*, void*);
        using ViewDeltaTime = float(__fastcall*)();
        using ViewEventDispatch = void(__fastcall*)(void*, void*, void*);
        ViewDispatch g_OriginalViewDispatch = nullptr;
        ViewSensitivity g_HorizontalSensitivity = nullptr, g_VerticalSensitivity = nullptr;
        ViewDeltaTime g_ViewDeltaTime = nullptr;
        ViewEventDispatch g_ViewEventDispatch = nullptr;
        const uint8_t* g_ViewDispatchHotfix = nullptr;
        float g_ViewMinimumSquared = 0, g_ViewRadians = 0, g_ViewTimeBias = 0;
        thread_local bool g_PreserveSmallViewInput = false;

        int InputMode()
        {
            __try
            {
                if (!g_InputRootSlot || !*g_InputRootSlot) return 0;
                auto* manager = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(*g_InputRootSlot) +
                    (Offsets::IsOS ? InputManagerSlotOS : InputManagerSlot));
                return manager ? *reinterpret_cast<int*>(manager +
                    (Offsets::IsOS ? InputModeFieldOS : InputModeField)) : 0;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
        }

        void __fastcall HookViewInput(void* self, uint64_t input, bool smooth, bool sensitivity, void* method)
        {
            const bool preserve = Config::Get().disable_camera_smooth && InputMode() == KeyboardMouseMode;
            const bool previous = g_PreserveSmallViewInput;
            g_PreserveSmallViewInput = preserve;
            if (preserve) smooth = false;
            __try { g_OriginalViewInput(self, input, smooth, sensitivity, method); }
            __finally { g_PreserveSmallViewInput = previous; }
        }

        bool IsSmallViewInput(uint64_t input)
        {
            float value[2]; memcpy(value, &input, sizeof(value));
            return std::isfinite(value[0]) && std::isfinite(value[1]) &&
                (value[0] != 0 || value[1] != 0) &&
                value[0] * value[0] + value[1] * value[1] < g_ViewMinimumSquared;
        }

        bool PrepareSmallViewInput(void* self)
        {
            __try
            {
                // Keep the native hotfix route and the two live-binding checks.
                if (!self || !g_ViewDispatchHotfix || *g_ViewDispatchHotfix) return false;
                auto* bytes = static_cast<uint8_t*>(self);
                auto* binding = *reinterpret_cast<uint8_t**>(bytes +
                    (Offsets::IsOS ? ViewInputBindingOS : ViewInputBinding));
                if (!binding || !*reinterpret_cast<void**>(binding + ViewBindingOwner) ||
                    !*reinterpret_cast<void**>(binding + ViewBindingTarget)) return false;
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        }

        void __fastcall HookViewDispatch(void* self, uint64_t input, void* method)
        {
            if (!g_PreserveSmallViewInput || !IsSmallViewInput(input) || !PrepareSmallViewInput(self))
            {
                g_OriginalViewDispatch(self, input, method);
                return;
            }
            // Only replace the native near-zero rejection. Use its getters,
            // float operation order, reusable event and dispatcher unchanged.
            float value[2]; memcpy(value, &input, sizeof(value));
            auto* bytes = static_cast<uint8_t*>(self);
            bytes[Offsets::IsOS ? ViewInputActiveOS : ViewInputActive] = 1;
            const auto event = Offsets::IsOS ? ViewInputEventOS : ViewInputEvent;
            const float horizontal = g_HorizontalSensitivity(self, nullptr);
            const float dx = ((-value[0] * horizontal) * g_ViewRadians) / (g_ViewDeltaTime() + g_ViewTimeBias);
            *reinterpret_cast<double*>(bytes + event + 8) = dx;
            const float vertical = g_VerticalSensitivity(self, nullptr);
            const float dy = ((-value[1] * vertical) * g_ViewRadians) / (g_ViewDeltaTime() + g_ViewTimeBias);
            *reinterpret_cast<double*>(bytes + event + 16) = dy;
            g_ViewEventDispatch(self, bytes + event, nullptr);
        }

        uint8_t* Relative(uint8_t* instruction, size_t displacement, size_t length)
        {
            int32_t relative = 0; memcpy(&relative, instruction + displacement, sizeof(relative));
            return instruction + length + relative;
        }

        bool FieldEquals(uint8_t* address, uint32_t expected)
        {
            uint32_t field = 0; memcpy(&field, address, sizeof(field)); return field == expected;
        }

        bool ResolveViewDispatch(uint8_t* dispatch)
        {
            auto* base = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
            if (!dispatch || !Compatibility::ResolveFunctionEntry(dispatch - base,
                    Patterns::CameraViewDispatch, ViewDispatchEntrySize) ||
                Scanner::ScanRange(dispatch + ViewDispatchGateCheck, ViewDispatchGateSize,
                    Patterns::CameraViewDispatchGate) != dispatch + ViewDispatchGateCheck ||
                Scanner::ScanRange(dispatch + ViewDispatchConvert, ViewDispatchConvertSize,
                    Patterns::CameraViewDispatchConvert) != dispatch + ViewDispatchConvert ||
                !FieldEquals(dispatch + ViewDispatchActiveWrite + 3, Offsets::IsOS ? ViewInputActiveOS : ViewInputActive) ||
                !FieldEquals(dispatch + ViewDispatchBindingRead + 3, Offsets::IsOS ? ViewInputBindingOS : ViewInputBinding) ||
                !FieldEquals(dispatch + ViewDispatchConvert + 3, Offsets::IsOS ? ViewInputEventOS : ViewInputEvent) ||
                !FieldEquals(dispatch + ViewDispatchXWrite + 4, (Offsets::IsOS ? ViewInputEventOS : ViewInputEvent) + 8) ||
                !FieldEquals(dispatch + ViewDispatchYWrite + 4, (Offsets::IsOS ? ViewInputEventOS : ViewInputEvent) + 16)) return false;

            auto* horizontal = Relative(dispatch + ViewHorizontalSensitivityCall, 1, 5);
            auto* vertical = Relative(dispatch + ViewVerticalSensitivityCall, 1, 5);
            auto* delta = Relative(dispatch + ViewDeltaTimeCall, 1, 5);
            auto* send = Relative(dispatch + ViewEventDispatchJump, 1, 5);
            if (Relative(dispatch + ViewDeltaTimeSecondCall, 1, 5) != delta ||
                !Compatibility::ResolveFunctionEntry(horizontal - base, Patterns::CameraViewSensitivityGetter, 9) ||
                !Compatibility::ResolveFunctionEntry(vertical - base, Patterns::CameraViewSensitivityGetter, 9) ||
                !Compatibility::ResolveFunctionEntry(delta - base, Patterns::CameraViewDeltaTimeGetter, 22) ||
                !Compatibility::ResolveFunctionEntry(send - base, Patterns::CameraViewEventDispatch, 15)) return false;

            memcpy(&g_ViewMinimumSquared, Relative(dispatch + ViewDispatchGateCheck, 4, 8), sizeof(float));
            memcpy(&g_ViewRadians, Relative(dispatch + ViewRadiansLoad, 5, 9), sizeof(float));
            memcpy(&g_ViewTimeBias, Relative(dispatch + ViewTimeBiasLoad, 5, 9), sizeof(float));
            if (g_ViewMinimumSquared != 0.00001f * 0.00001f ||
                g_ViewRadians != 0.017453292f || g_ViewTimeBias != 0.0001f) return false;
            g_HorizontalSensitivity = reinterpret_cast<ViewSensitivity>(horizontal);
            g_VerticalSensitivity = reinterpret_cast<ViewSensitivity>(vertical);
            g_ViewDeltaTime = reinterpret_cast<ViewDeltaTime>(delta);
            g_ViewEventDispatch = reinterpret_cast<ViewEventDispatch>(send);
            g_ViewDispatchHotfix = Relative(dispatch + ViewDispatchHotfixCheck, 2, 7);
            return true;
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
        // This is the observed mouse-input filter before camera event dispatch.
        // Native sensitivity, inversion, TPS and output delivery remain native.
        using namespace Offsets::CameraTweaks;
        auto* input = static_cast<uint8_t*>(Scanner::ScanUniqueMainMod(Patterns::CameraViewInput));
        auto* dispatch = static_cast<uint8_t*>(Scanner::ScanUniqueMainMod(Patterns::CameraViewDispatch));
        auto* base = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
        const auto* rootPattern = Offsets::IsOS ? Patterns::CameraViewInputRootOS : Patterns::CameraViewInputRoot;
        const auto* filterPattern = Offsets::IsOS ? Patterns::CameraViewInputFilterOS : Patterns::CameraViewInputFilter;
        if (input && Compatibility::ResolveFunctionEntry(input - base, Patterns::CameraViewInput, ViewInputRootLoad) &&
            Scanner::ScanRange(input + ViewInputRootLoad, ViewInputRootLoadSize, rootPattern) == input + ViewInputRootLoad &&
            Scanner::ScanRange(input + ViewInputFilterCheck, ViewInputFilterCheckSize, filterPattern) == input + ViewInputFilterCheck &&
            ResolveViewDispatch(dispatch))
        {
            int32_t relative = 0;
            memcpy(&relative, input + ViewInputRootLoad + 3, sizeof(relative));
            g_InputRootSlot = reinterpret_cast<void**>(input + ViewInputRootLoad + 7 + relative);
            if (MH_CreateHook(dispatch, reinterpret_cast<void*>(HookViewDispatch),
                reinterpret_cast<void**>(&g_OriginalViewDispatch)) == MH_OK)
            {
                if (MH_CreateHook(input, reinterpret_cast<void*>(HookViewInput),
                    reinterpret_cast<void**>(&g_OriginalViewInput)) == MH_OK)
                    std::cout << "[SCAN] Camera view-input filter and small-input dispatch hooks ready.\n";
                else
                {
                    MH_RemoveHook(dispatch);
                    g_OriginalViewDispatch = nullptr;
                    std::cout << "[WARN] Camera view-input hook failed; dispatch hook removed.\n";
                }
            }
            else std::cout << "[WARN] Camera view-input hook failed; native filtering retained.\n";
        }
        else std::cout << "[WARN] Camera view-input path unsupported; native filtering retained.\n";

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
