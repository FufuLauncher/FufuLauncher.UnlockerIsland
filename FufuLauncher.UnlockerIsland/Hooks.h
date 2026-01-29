#pragma once

namespace Hooks {
    bool Init();
    void Uninit();
    
    bool IsGameUpdateInit();
    void RequestOpenCraft();
    void TriggerReloadPopup();
    void UpdateVisuals();
    void InitHSRFps();   
    void UpdateHSRFps();
}