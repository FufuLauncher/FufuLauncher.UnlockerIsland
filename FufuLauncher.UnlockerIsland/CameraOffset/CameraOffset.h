/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once

namespace CameraOffset
{
    struct Offset
    {
        double x = 0, y = 0, z = 0;
    };

    struct Rotation
    {
        double x, y, z, w;
    };

    void Init();
    void SuspendImmediately();
    void Tick(bool allowGameplayCameraOffset, bool cameraOwnedByAnotherFeature);
    bool HasPendingOffset();
    bool GetWorldOffset(double dt, const Rotation& rotation, Offset& result);
}
