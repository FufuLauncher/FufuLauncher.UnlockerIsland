/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "Camera.h"

#include "../Patterns/Patterns.h"
#include "../Scanner/Scanner.h"
#include "../il2cpp/Il2CppArray.h"

#include <atomic>
#include <iostream>

namespace Camera
{
    namespace
    {
        using FnGetMain = void* (__fastcall*)();
        using FnGetAllCameras = Il2CppArray<void*>* (__fastcall*)();
        using FnGetTransform = void* (__fastcall*)(void*);
        using FnSetPosition = void (__fastcall*)(void*, Vector3*);
        using FnSetRotation = void (__fastcall*)(void*, Quaternion*);
        using FnGetPosition = void (__fastcall*)(Vector3*, void*);
        using FnGetRotation = void (__fastcall*)(Quaternion*, void*);

        FnGetMain g_fnGetMain = nullptr;
        FnGetAllCameras g_fnGetAllCameras = nullptr;
        FnGetTransform g_fnGetTransform = nullptr;
        FnSetPosition g_fnSetPosition = nullptr;
        FnSetRotation g_fnSetRotation = nullptr;
        FnGetPosition g_fnGetPosition = nullptr;
        FnGetRotation g_fnGetRotation = nullptr;

        void* g_Transform = nullptr;
        void* g_SecondaryTransform = nullptr;
        std::atomic<bool> g_Ready{false};
        ULONGLONG g_LastRefresh = 0;

        void* SafeGetMain()
        {
            if (!g_fnGetMain) return nullptr;
            __try { return g_fnGetMain(); }
            __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
        }

        void* SafeGetTransform(void* camera)
        {
            if (!g_fnGetTransform || !camera) return nullptr;
            __try { return g_fnGetTransform(camera); }
            __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
        }

        Il2CppArray<void*>* SafeGetAllCameras()
        {
            if (!g_fnGetAllCameras) return nullptr;
            __try { return g_fnGetAllCameras(); }
            __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
        }

        size_t SafeGetCameraCount(Il2CppArray<void*>* cameras)
        {
            if (!cameras) return 0;
            __try { return cameras->Count(); }
            __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
        }

        bool SafeGetCameraAt(Il2CppArray<void*>* cameras, size_t index, void*& camera)
        {
            __try
            {
                if (!cameras || index >= cameras->Count()) return false;
                camera = cameras->Get(static_cast<int>(index));
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        }
    }

    Quaternion Multiply(const Quaternion& a, const Quaternion& b)
    {
        return {
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
        };
    }

    Vector3 RotateVector(const Quaternion& rotation, const Vector3& vector)
    {
        Quaternion qv{vector.x, vector.y, vector.z, 0.0f};
        Quaternion conjugate{-rotation.x, -rotation.y, -rotation.z, rotation.w};
        Quaternion result = Multiply(Multiply(rotation, qv), conjugate);
        return {result.x, result.y, result.z};
    }

    Quaternion FromYawPitch(float yawDegrees, float pitchDegrees)
    {
        float yaw = yawDegrees * 0.0174532925f * 0.5f;
        float pitch = pitchDegrees * 0.0174532925f * 0.5f;
        Quaternion yawRotation{0, sinf(yaw), 0, cosf(yaw)};
        Quaternion pitchRotation{sinf(pitch), 0, 0, cosf(pitch)};
        return Multiply(yawRotation, pitchRotation);
    }

    bool Init()
    {
        std::cout << "[SCAN] Initializing shared camera access..." << std::endl;

        void* getMain = Scanner::ScanMainMod(Patterns::FreeCamCameraGetMain);
        void* getTransform = Scanner::ScanMainMod(Patterns::FreeCamComponentGetTransform);
        void* getPosition = Scanner::ScanMainMod(Patterns::FreeCamTransformGetPosition);
        void* setPosition = Scanner::ScanMainMod(Patterns::FreeCamTransformSetPosition);
        void* setRotation = Scanner::ScanMainMod(Patterns::FreeCamTransformSetRotation);
        void* getRotation = Scanner::ScanMainMod(Patterns::FreeCamTransformGetRotation);

        if (!getMain || !getTransform || !getPosition || !setPosition || !setRotation)
        {
            std::cout << "   -> [ERR] Shared camera patterns not found; camera features are disabled." << std::endl;
            return false;
        }

        g_fnGetMain = reinterpret_cast<FnGetMain>(getMain);
        // In 7.1 Camera.main is the UI camera; the adjacent thunk enumerates
        // the active cameras so the gameplay camera can be selected instead.
        g_fnGetAllCameras = reinterpret_cast<FnGetAllCameras>(
            static_cast<unsigned char*>(getMain) + 0x10);
        g_fnGetTransform = reinterpret_cast<FnGetTransform>(getTransform);
        g_fnGetPosition = reinterpret_cast<FnGetPosition>(getPosition);
        g_fnSetPosition = reinterpret_cast<FnSetPosition>(setPosition);
        g_fnSetRotation = reinterpret_cast<FnSetRotation>(setRotation);
        g_fnGetRotation = reinterpret_cast<FnGetRotation>(getRotation);
        g_Ready.store(true, std::memory_order_relaxed);

        std::cout << "   -> Shared camera access ready." << std::endl;
        if (!g_fnGetRotation)
        {
            std::cout <<
                "   -> [WARN] Camera rotation pattern not found; horizontal camera-relative offsets are disabled." <<
                std::endl;
        }
        return true;
    }

    void Tick()
    {
        if (!IsReady()) return;

        ULONGLONG now = GetTickCount64();
        if (now - g_LastRefresh <= 2000) return;
        g_LastRefresh = now;

        void* camera = SafeGetMain();
        g_Transform = nullptr;
        g_SecondaryTransform = nullptr;
        auto* cameras = SafeGetAllCameras();
        size_t count = SafeGetCameraCount(cameras);
        if (camera && count > 0 && count <= 16)
        {
            for (size_t i = 0; i < count; ++i)
            {
                void* item = nullptr;
                if (!SafeGetCameraAt(cameras, i, item) || !item || item == camera) continue;
                void* transform = SafeGetTransform(item);
                if (!transform) continue;
                if (!g_Transform)
                {
                    g_Transform = transform;
                }
                else if (!g_SecondaryTransform && transform != g_Transform)
                {
                    g_SecondaryTransform = transform;
                }
            }
        }
    }

    void Invalidate()
    {
        g_Transform = nullptr;
        g_SecondaryTransform = nullptr;
        g_LastRefresh = 0;
    }

    bool IsReady()
    {
        return g_Ready.load(std::memory_order_relaxed);
    }

    void* GetTransform()
    {
        return g_Transform;
    }

    bool GetPosition(Vector3& outPosition)
    {
        if (!g_fnGetPosition || !g_Transform) return false;
        __try
        {
            g_fnGetPosition(&outPosition, g_Transform);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool GetRotation(Quaternion& outRotation)
    {
        if (!g_fnGetRotation || !g_Transform) return false;
        __try
        {
            g_fnGetRotation(&outRotation, g_Transform);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool SetPosition(const Vector3& position)
    {
        if (!g_fnSetPosition || !g_Transform) return false;
        Vector3 value = position;
        __try
        {
            g_fnSetPosition(g_Transform, &value);
            if (g_SecondaryTransform) g_fnSetPosition(g_SecondaryTransform, &value);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool SetRotation(const Quaternion& rotation)
    {
        if (!g_fnSetRotation || !g_Transform) return false;
        Quaternion value = rotation;
        __try
        {
            g_fnSetRotation(g_Transform, &value);
            if (g_SecondaryTransform) g_fnSetRotation(g_SecondaryTransform, &value);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
}
