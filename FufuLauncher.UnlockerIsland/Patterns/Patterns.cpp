/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "Patterns.h"
#include <iostream>

std::string GetOwnDllDir();

namespace Offsets {
    std::string GetActiveOffset;
    std::string GetComponent;
    std::string GetText;
    std::string ClockPageOkOffset;
    std::string ClockPageCloseOffset;
    std::string ClockPageFinishOffset;
    std::string ClockPageBackOffset;
    std::string TouchInputOffset;
    std::string InnerDispatcherOffset;
    std::string EventCameraOffset;
    std::string DamageColorAOffset;
    std::string DamageColorBOffset;
    std::string DamageColor1Offset;
    std::string DamageColor2Offset;
    std::string DamageColor3Offset;
    std::string DamageColor4Offset;
    std::string UpdateInnerTargetOffset;
    std::string SetActiveOffset;
    std::string ProfilePageRefreshOffset;

    void InitOffsets(bool isOS) {
        if (isOS) {
            GetActiveOffset = Patterns::OS::GetActiveOffset;
            GetComponent = Patterns::OS::GetComponent;
            GetText = Patterns::OS::GetText;
            ClockPageOkOffset = Patterns::OS::ClockPageOkOffset;
            ClockPageCloseOffset = Patterns::OS::ClockPageCloseOffset;
            ClockPageFinishOffset = Patterns::OS::ClockPageFinishOffset;
            ClockPageBackOffset = Patterns::OS::ClockPageBackOffset;
            TouchInputOffset = Patterns::OS::TouchInputOffset;
            InnerDispatcherOffset = Patterns::OS::InnerDispatcherOffset;
            EventCameraOffset = Patterns::OS::EventCameraOffset;
            DamageColorAOffset = Patterns::OS::DamageColorA;
            DamageColorBOffset = Patterns::OS::DamageColorB;
            DamageColor1Offset = Patterns::OS::DamageColor1;
            DamageColor2Offset = Patterns::OS::DamageColor2;
            DamageColor3Offset = Patterns::OS::DamageColor3;
            DamageColor4Offset = Patterns::OS::DamageColor4;
            UpdateInnerTargetOffset = Patterns::OS::UpdateInnerTargetOffset;
            SetActiveOffset = Patterns::OS::SetActiveOffset;
            std::cout << "[INFO] Pre-initialized Global (OS) Offsets from hardcode" << std::endl;
        } else {
            GetActiveOffset = Patterns::CN::GetActiveOffset;
            GetComponent = Patterns::CN::GetComponent;
            GetText = Patterns::CN::GetText;
            ClockPageOkOffset = Patterns::CN::ClockPageOkOffset;
            ClockPageCloseOffset = Patterns::CN::ClockPageCloseOffset;
            ClockPageFinishOffset = Patterns::CN::ClockPageFinishOffset;
            ClockPageBackOffset = Patterns::CN::ClockPageBackOffset;
            TouchInputOffset = Patterns::CN::TouchInputOffset;
            InnerDispatcherOffset = Patterns::CN::InnerDispatcherOffset;
            EventCameraOffset = Patterns::CN::EventCameraOffset;
            DamageColorAOffset = Patterns::CN::DamageColorA;
            DamageColorBOffset = Patterns::CN::DamageColorB;
            DamageColor1Offset = Patterns::CN::DamageColor1;
            DamageColor2Offset = Patterns::CN::DamageColor2;
            DamageColor3Offset = Patterns::CN::DamageColor3;
            DamageColor4Offset = Patterns::CN::DamageColor4;
            UpdateInnerTargetOffset = Patterns::CN::UpdateInnerTargetOffset;
            SetActiveOffset = Patterns::CN::SetActiveOffset;
            ProfilePageRefreshOffset = Patterns::CN::ProfilePageRefreshOffset;
            std::cout << "[INFO] Pre-initialized China (CN) Offsets from hardcode" << std::endl;
        }
    }
}