/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "CameraOffset.h"
#include "../Config/Config.h"
#include <algorithm>
#include <atomic>
#include <cmath>

namespace CameraOffset
{
    namespace
    {
        std::atomic<bool> g_Allowed{false};
        Offset g_Current{};
        Offset g_Right{1, 0, 0};
        bool g_BasisValid = false;

        bool Finite(const Offset& v)
        {
            return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
        }

        bool UpdateBasis(const Rotation& q)
        {
            const double norm = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
            if (!std::isfinite(norm) || norm < 0.25 || norm > 4.0) return false;
            // Horizontal camera right; Y stays world-up and Z stays backward.
            const double x = 1.0 - 2.0 * (q.y * q.y + q.z * q.z) / norm;
            const double z = 2.0 * (q.x * q.z - q.w * q.y) / norm;
            const double length = std::hypot(x, z);
            if (length < 0.001) return g_BasisValid;
            g_Right = {x / length, 0.0, z / length};
            g_BasisValid = true;
            return true;
        }
    }

    void Init() { SuspendImmediately(); }

    void SuspendImmediately()
    {
        g_Allowed.store(false, std::memory_order_relaxed);
        g_Current = {};
        g_BasisValid = false;
    }

    void Tick(bool allowGameplayCameraOffset, bool cameraOwnedByAnotherFeature)
    {
        // ChangeFOV only updates eligibility. Interpolation belongs to the
        // native per-frame callback, not to Unity transform writes.
        if (!allowGameplayCameraOffset || cameraOwnedByAnotherFeature)
        {
            SuspendImmediately();
        }
        else
        {
            g_Allowed.store(true, std::memory_order_relaxed);
        }
    }

    bool HasPendingOffset()
    {
        return Config::Get().enable_camera_offset || std::abs(g_Current.x) > 0.0005 ||
            std::abs(g_Current.y) > 0.0005 || std::abs(g_Current.z) > 0.0005;
    }

    bool GetWorldOffset(double dt, const Rotation& rotation, Offset& result)
    {
        result = {};
        if (!g_Allowed.load(std::memory_order_relaxed)) return false;
        const auto& cfg = Config::Get();
        Offset target{};
        if (cfg.enable_camera_offset)
        {
            target = {cfg.camera_offset_x, cfg.camera_offset_y, cfg.camera_offset_z};
        }
        if (!Finite(target) || !std::isfinite(dt) || !UpdateBasis(rotation))
        {
            g_Current = {};
            g_BasisValid = false;
            return false;
        }
        const double speed = std::isfinite(cfg.camera_height_transition_speed)
                                 ? std::clamp(static_cast<double>(cfg.camera_height_transition_speed), 0.1, 30.0)
                                 : 8.0;
        const double blend = cfg.disable_camera_blend ? 1.0 : 1.0 - std::exp(-speed * std::clamp(dt, 0.0, 0.1));
        g_Current.x += (target.x - g_Current.x) * blend;
        g_Current.y += (target.y - g_Current.y) * blend;
        g_Current.z += (target.z - g_Current.z) * blend;
        if (std::abs(g_Current.x - target.x) <= 0.0005 &&
            std::abs(g_Current.y - target.y) <= 0.0005 &&
            std::abs(g_Current.z - target.z) <= 0.0005)
            g_Current = target;
        result = {
            g_Right.x * g_Current.x + g_Right.z * g_Current.z,
            g_Current.y, g_Right.z * g_Current.x - g_Right.x * g_Current.z
        };
        return std::abs(result.x) > 0.0005 || std::abs(result.y) > 0.0005 ||
            std::abs(result.z) > 0.0005;
    }
}
