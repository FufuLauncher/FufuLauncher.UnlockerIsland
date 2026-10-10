/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma
#include <atomic>

extern std::atomic<bool> g_StopDialogPolling;

namespace Hooks
{
    bool Init();
    void Uninit();

    bool IsGameUpdateInit();
    bool CanApplyCameraOffset();
    void RequestOpenCraft();
    void TriggerReloadPopup();
    void UpdateVisuals();
}
