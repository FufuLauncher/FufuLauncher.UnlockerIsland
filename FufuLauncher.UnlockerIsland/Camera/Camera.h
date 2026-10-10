/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once

#include "../Core/SharedState.h"

namespace Camera
{
    struct Quaternion
    {
        float x, y, z, w;
    };

    bool Init();
    void Tick();
    void Invalidate();
    bool IsReady();
    void* GetTransform();

    bool GetPosition(Vector3& outPosition);
    bool GetRotation(Quaternion& outRotation);
    bool SetPosition(const Vector3& position);
    bool SetRotation(const Quaternion& rotation);

    Quaternion Multiply(const Quaternion& a, const Quaternion& b);
    Vector3 RotateVector(const Quaternion& rotation, const Vector3& vector);
    Quaternion FromYawPitch(float yawDegrees, float pitchDegrees);
}
