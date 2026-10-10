/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "CameraCollision.h"
#include "../CameraOffset/CameraOffset.h"
#include "../Config/Config.h"
#include "../Core/Hooks.h"
#include "../FreeCamera/FreeCamera.h"
#include "../MinHook/MinHook.h"
#include "../Patterns/Patterns.h"
#include "../Scanner/Scanner.h"
#include "../Visual/Visual.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace CameraCollision {
    namespace {
        // CN 7.1 DAMLEAKHMIP.OFLJEMLABKC, the common native camera protector.
        using Tick = void(__fastcall*)(void*, double, void*, void*, void*, void*, void*);
        Tick g_Tick = nullptr;
        using Vector = CameraOffset::Offset;
        constexpr size_t kDesiredPosition = 0x60; // unboxed MKEKBINNCNJ
        constexpr size_t kCameraMode = 0x240;
        constexpr size_t kSkipProtection = 0x291;
        constexpr size_t kEventTarget = 0x293;
        constexpr size_t kPosition = 0x30; // unboxed CameraStateData
        constexpr size_t kOrientation = 0x48;
        constexpr size_t kLookAt = 0xA8;

        struct CallState {
            uint8_t* input = nullptr;
            uint8_t originalSkip = 0;
            bool skipWritten = false;
        };

        bool Finite(const Vector& v) {
            return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
        }

        Vector Shift(const Vector& v, const Vector& offset) {
            return { v.x + offset.x, v.y + offset.y, v.z + offset.z };
        }

        void PrepareCall(void* input, void* output, double dt, CallState& call) {
            if (!Config::Get().disable_camera_collision && !CameraOffset::HasPendingOffset()) return;
            if (!input || !output || FreeCamera::IsActive() || IsCameraPageActiveFromEvents()) {
                CameraOffset::SuspendImmediately();
                return;
            }
            __try {
                auto* state = static_cast<uint8_t*>(input);
                auto* pose = static_cast<uint8_t*>(output);
                // Normal follow cameras share this protector, including combat,
                // climb and flight. Parallel/Cinema are separate native modes.
                if (*reinterpret_cast<int*>(state + kCameraMode) != 0) {
                    CameraOffset::SuspendImmediately();
                    return;
                }
                const auto skip = state[kSkipProtection];
                if (skip > 1 || state[kEventTarget] > 1) return;

                if (CameraOffset::HasPendingOffset()) {
                    const double fov = *reinterpret_cast<double*>(pose);
                    if (std::isfinite(fov) && fov > 30.0 && state[kEventTarget] == 0) {
                        // Check cached UI eligibility on the actual frame callback,
                        // so leaving aim/dialogue does not require another FOV event.
                        CameraOffset::Tick(Hooks::CanApplyCameraOffset(), false);
                        CameraOffset::Rotation rotation;
                        std::memcpy(&rotation, pose + kOrientation, sizeof(rotation));
                        Vector offset;
                        if (CameraOffset::GetWorldOffset(dt, rotation, offset)) {
                            Vector desired, position, lookAt;
                            std::memcpy(&desired, state + kDesiredPosition, sizeof(desired));
                            std::memcpy(&position, pose + kPosition, sizeof(position));
                            std::memcpy(&lookAt, pose + kLookAt, sizeof(lookAt));
                            // Translate the desired pose and its look-at point as a
                            // pair. Spherical angles, radius and orientation remain
                            // native; the offset does not change their desired values.
                            desired = Shift(desired, offset);
                            position = Shift(position, offset);
                            lookAt = Shift(lookAt, offset);
                            if (Finite(desired) && Finite(position) && Finite(lookAt)) {
                                std::memcpy(state + kDesiredPosition, &desired, sizeof(desired));
                                std::memcpy(pose + kPosition, &position, sizeof(position));
                                std::memcpy(pose + kLookAt, &lookAt, sizeof(lookAt));
                            }
                        }
                    } else {
                        // Retain the offset module's existing aim/script exclusions.
                        CameraOffset::SuspendImmediately();
                    }
                }

                if (Config::Get().disable_camera_collision) {
                    call.input = state;
                    call.originalSkip = skip;
                    state[kSkipProtection] = 1;
                    call.skipWritten = true;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                // Unsupported/invalid state keeps the original native call.
            }
        }

        void __fastcall HookTick(void* self, double dt, void* input, void* output,
            void* extra, void* context, void* method) {
            CallState call;
            PrepareCall(input, output, dt, call);
            __try {
                // Offset is already in the desired pose. Walls/ground, radius
                // recovery, near-plane handling and damping resolve it natively.
                g_Tick(self, dt, input, output, extra, context, method);
            } __finally {
                if (call.skipWritten) call.input[kSkipProtection] = call.originalSkip;
            }
        }
    }

    void Init() {
        auto* tick = Scanner::ScanMainMod(Patterns::CameraCollisionTick);
        if (!tick || !Scanner::ScanRange(tick, 0x100,
                "80 BE 91 02 00 00 00 0F 85 ? ? ? ? 80 BB 36 04 00 00 00") ||
            !Scanner::ScanRange(tick, 0x100,
                "0F 10 87 A8 00 00 00 48 8B 87 B8 00 00 00 48 89 83 98 04 00 00 0F 11 83 88 04 00 00")) {
            std::cout << "[WARN] Native camera protector layout unsupported; offset/collision options unavailable.\n";
            return;
        }
        if (MH_CreateHook(tick, reinterpret_cast<void*>(HookTick),
                reinterpret_cast<void**>(&g_Tick)) != MH_OK) {
            std::cout << "[WARN] Native camera protector hook failed.\n";
            return;
        }
        // Always install, including when both options are initially disabled.
        std::cout << "[SCAN] Native third-person camera offset/collision hook ready (experimental).\n";
    }
}
