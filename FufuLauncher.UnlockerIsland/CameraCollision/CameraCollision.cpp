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

namespace CameraCollision
{
    namespace
    {
        // The common native camera protector; input/output pose fields are shared.
        using Tick = void(__fastcall*)(void*, double, void*, void*, void*, void*, void*);
        Tick g_Tick = nullptr;
        using Vector = CameraOffset::Offset;
        using namespace Offsets::CameraCollision;

        struct CallState
        {
            uint8_t* input = nullptr;
            uint8_t originalSkip = 0;
            bool skipWritten = false;
        };

        bool Finite(const Vector& v)
        {
            return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
        }

        Vector Shift(const Vector& v, const Vector& offset)
        {
            return {v.x + offset.x, v.y + offset.y, v.z + offset.z};
        }

        void PrepareCall(void* input, void* output, double dt, CallState& call)
        {
            if (!Config::Get().disable_camera_collision && !CameraOffset::HasPendingOffset()) return;
            if (!input || !output || FreeCamera::IsActive() || IsCameraPageActiveFromEvents())
            {
                CameraOffset::SuspendImmediately();
                return;
            }
            __try
            {
                auto* state = static_cast<uint8_t*>(input);
                auto* pose = static_cast<uint8_t*>(output);
                // Normal follow cameras share this protector, including combat,
                // climb and flight. Parallel/Cinema are separate native modes.
                if (*reinterpret_cast<int*>(state + CameraMode) != 0)
                {
                    CameraOffset::SuspendImmediately();
                    return;
                }
                const auto skip = state[SkipProtection];
                if (skip > 1 || state[EventTarget] > 1) return;

                if (CameraOffset::HasPendingOffset())
                {
                    const double fov = *reinterpret_cast<double*>(pose);
                    if (std::isfinite(fov) && fov > 30.0 && state[EventTarget] == 0)
                    {
                        // Check cached UI eligibility on the actual frame callback,
                        // so leaving aim/dialogue does not require another FOV event.
                        CameraOffset::Tick(Hooks::CanApplyCameraOffset(), false);
                        CameraOffset::Rotation rotation;
                        std::memcpy(&rotation, pose + Orientation, sizeof(rotation));
                        Vector offset;
                        if (CameraOffset::GetWorldOffset(dt, rotation, offset))
                        {
                            Vector desired, position, lookAt;
                            std::memcpy(&desired, state + DesiredPosition, sizeof(desired));
                            std::memcpy(&position, pose + Position, sizeof(position));
                            std::memcpy(&lookAt, pose + LookAt, sizeof(lookAt));
                            // Translate the desired pose and its look-at point as a
                            // pair. Spherical angles, radius and orientation remain
                            // native; the offset does not change their desired values.
                            desired = Shift(desired, offset);
                            position = Shift(position, offset);
                            lookAt = Shift(lookAt, offset);
                            if (Finite(desired) && Finite(position) && Finite(lookAt))
                            {
                                std::memcpy(state + DesiredPosition, &desired, sizeof(desired));
                                std::memcpy(pose + Position, &position, sizeof(position));
                                std::memcpy(pose + LookAt, &lookAt, sizeof(lookAt));
                            }
                        }
                    }
                    else
                    {
                        // Retain the offset module's existing aim/script exclusions.
                        CameraOffset::SuspendImmediately();
                    }
                }

                if (Config::Get().disable_camera_collision)
                {
                    call.input = state;
                    call.originalSkip = skip;
                    state[SkipProtection] = 1;
                    call.skipWritten = true;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                // Unsupported/invalid state keeps the original native call.
            }
        }

        void __fastcall HookTick(void* self, double dt, void* input, void* output,
                                 void* extra, void* context, void* method)
        {
            CallState call;
            PrepareCall(input, output, dt, call);
            __try
            {
                // Offset is already in the desired pose. Walls/ground, radius
                // recovery, near-plane handling and damping resolve it natively.
                g_Tick(self, dt, input, output, extra, context, method);
            }
            __finally
            {
                if (call.skipWritten) call.input[SkipProtection] = call.originalSkip;
            }
        }
    }

    void Init()
    {
        using namespace Offsets::CameraCollision;
        auto* tick = Scanner::ScanMainMod(Patterns::CameraCollisionTick);
        if (!tick || !Scanner::ScanRange(tick, EntryCheckSize,
                Offsets::IsOS
                    ? Patterns::CameraCollisionGuardOS
                    : Patterns::CameraCollisionGuard) ||
            !Scanner::ScanRange(tick, EntryCheckSize,
                Offsets::IsOS
                    ? Patterns::CameraCollisionLookAtOS
                    : Patterns::CameraCollisionLookAt))
        {
            std::cout << "[WARN] Native camera protector layout unsupported; offset/collision options unavailable.\n";
            return;
        }
        if (MH_CreateHook(tick, reinterpret_cast<void*>(HookTick),
                          reinterpret_cast<void**>(&g_Tick)) != MH_OK)
        {
            std::cout << "[WARN] Native camera protector hook failed.\n";
            return;
        }
        // Always install, including when both options are initially disabled.
        std::cout << "[SCAN] Native third-person camera offset/collision hook ready (experimental).\n";
    }
}
