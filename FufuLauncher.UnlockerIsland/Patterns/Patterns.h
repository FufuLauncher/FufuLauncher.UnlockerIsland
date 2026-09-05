/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once
#include <string>

namespace Patterns {
    inline constexpr const char* GetFrameCount = "E8 ? ? ? ? 85 C0 7E 0E E8 ? ? ? ? 0F 57 C0 F3 0F 2A C0 EB 08";
    inline constexpr const char* SetFrameCount = "E8 ? ? ? ? E8 ? ? ? ? 83 F8 1F 0F 9C 05 ? ? ? ? 48 8B 05";
    inline constexpr const char* ChangeFOV = "40 53 48 83 EC 60 0F 29 74 24 ? 48 8B D9 0F 28 F1 E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? E8 ? ? ? ? 48 8B C8";
    inline constexpr const char* SwitchInputDeviceToTouchScreen = "56 57 48 83 EC ? 48 89 CE 80 3D ? ? ? ? 00 48 8B 05 ? ? ? ? 0F 85 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9 0F 84 ? ? ? ? 48 8B 15 ? ? ? ? E8 ? ? ? ? 48 89 C7 48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9 0F 84 ? ? ? ? 31 D2";
    inline constexpr const char* QuestBanner = "41 57 41 56 56 57 55 53 48 81 EC E8 00 00 00 0F 29 BC 24 ? ? ? ? 0F 29 B4 24 ? ? ? ? 48 89 CE 0F 57 C0 0F 29 84 24 ? ? ? ? 0F 29 84 24 ? ? ? ? 0F 29 84 24";
    inline constexpr const char* FindGameObject = "40 53 48 83 EC ? 48 89 4C 24 ? 48 8D 54 24 ? 48 8D 4C 24 ? E8 ? ? ? ? 48 8B 08 48 85 C9 75 ? 48 8D 48 ? E8 ? ? ? ? 48 8B 4C 24 ? 48 8B D8 48 85 C9 74 ? 48 83 7C 24 ? 00 76";
    inline constexpr const char* SetActive = ""; //removed, use offset
    inline constexpr const char* ProfilePageRefresh = "55 41 57 41 56 41 55 41 54 56 57 53 48 81 EC A8 00 00 00 48 8D AC 24 80 00 00 00 0F 29 75 10 48 C7 45 F8 FE FF FF FF 49 89 CC 80 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? 00 48 8B 05 ?? ?? ?? ?? 0F 85 ?? ?? ?? ??";
    inline constexpr const char* DamageText = "41 57 41 56 41 55 41 54 56 57 55 53 48 81 EC E8 01 00 00 44 0F 29 AC 24 D0 01 00 00 44 0F 29 A4 24 C0 01 00 00 44 0F 29 9C 24 B0 01 00 00 44 0F 29 94 24 A0 01 00 00 44 0F 29 8C 24 90 01 00 00 44 0F 29 84 24 80 01 00 00 0F 29 BC 24 70 01 00 00 0F 29 B4 24 60 01 00 00 44 89 CF 44 89 44 24 7C 89 D5 48 89 CE F3 44 0F 10 94 24 80 02 00 00 44 0F B6 A4 24 78 02 00 00"; 
    inline constexpr const char* EventCamera = "41 57 41 56 56 57 55 53 48 83 EC 48 48 89 D7 49 89 CE 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 80";
    inline constexpr const char* FindString = "56 48 83 ec 20 48 89 ce e8 ? ? ? ? 48 89 f1 89 c2 48 83 c4 20 5e e9 ? ? ? ? cc cc cc cc";
    inline constexpr const char* CraftPartner = "41 57 41 56 41 55 41 54 56 57 55 53 48 81 EC ? ? ? ? 4D 89 ? 4C 89 C6 49 89 D4 49 89 CE";
    inline constexpr const char* CraftEntry = "41 56 56 57 53 48 83 EC 48 49 89 CE 80 3D ? ? ? ? 00 0F 84 ? ? ? ? 80 3D ? ? ? ? 00 48 8B 0D ? ? ? ? 0F 85 ? ? ? ? 48 8B 81 ? ? ? ? 48 85 C0 0F 84"; 
    inline constexpr const char* CheckCanEnter = "56 48 81 ec 80 00 00 00 80 3d ? ? ? ? 00 0f 84 ? ? ? ? 80 3d ? ? ? ? 00";
    inline constexpr const char* OpenTeamPage = "56 57 53 48 83 ec 20 89 cb 80 3d ? ? ? ? 00 74 7a 80 3d ? ? ? ? 00 48 8b 05";
    inline constexpr const char* OpenTeam = "48 83 EC 28 80 3D ? ? ? ? 00 75 ? 48 8B 0D ? ? ? ? 80 B9 C7 00 00 00 00 74 ? B9 0C 00 00 00 E8 ? ? ? ? 84 C0 74"; 
    inline constexpr const char* DisplayFog = "0F B6 02 88 01 8B 42 04 89 41 04 F3 0F 10 52 ? F3 0F 10 4A ? F3 0F 10 42 ? 8B 42 08";
    inline constexpr const char* PlayerPerspective = "E8 ? ? ? ? 48 8B BE ? ? ? ? 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 80 BE ? ? ? ? ? 74 11";
    inline constexpr const char* SetSyncCount = "E8 ? ? ? ? E8 ? ? ? ? 89 C6 E8 ? ? ? ? 31 C9 89 F2 49 89 C0 E8 ? ? ? ? 48 89 C6 48 8B 0D ? ? ? ? 80 B9 ? ? ? ? ? 74 47 48 8B 3D ? ? ? ? 48 85 DF 74 4C";
    inline constexpr const char* GameUpdate = "55 56 57 53 48 83 EC ? 48 8D 6C 24 ? 48 C7 45 ? ? ? ? ? 48 8B 41 ? 48 85 C0 0F 84 ? ? ? ? 83 78";
    inline constexpr const char* CheckCanOpenMap = "E8 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? ?? ?? 48 8B 45 ?? 48 85 C0 74 ?? 41 8B 17 4C 8B 40 ?? 48 8B 48 ?? FF 50 ?? 84 C0 0F 84 ?? ?? ?? ??";
    inline constexpr const char* GetName = "40 53 48 81 EC ?? ?? ?? ?? 48 8B D9 48 85 C9 0F 84 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0 0F 84 ?? ?? ?? ?? 48 8B 10 48 8B C8 FF 52 ?? 48 85 C0 0F 85 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ??";
    inline constexpr const char* GetActive = "E8 ?? ?? ?? ?? 84 C0 74 ?? 48 89 F1 E8 ?? ?? ?? ?? 48 8B 4E ?? 48 85 C9 0F 84 ?? ?? ?? ?? 80 79 ?? ?? 0F 94 C1 08 C1";
    inline constexpr const char* AvatarPaimonAppear = "41 56 56 57 53 48 83 EC 28 44 89 C3 48 89 D6 49 89 CE 80 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 80 B9 C7 00 00 00 00 0F 84 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0 0F 84 ?? ?? ?? ??";
    inline constexpr const char* StringNew = "56 48 83 EC 20 48 85 C9 74 ? 48 89 CE E8 ? ? ? ? 48 89 F1 89 C2";
    inline constexpr const char* ShowDialog = "41 57 41 56 56 57 55 53 48 83 EC 28 4D 89 CF 4C 89 C7 48 89 D5 48 89 CB";
    inline constexpr const char* CookHandler = "41 56 56 57 55 53 48 83 EC 20 48 89 D3 49 89 CE 80 3D ? ? ? ? 00 0F 84 ? ? ? ? 80 3D ? ? ? ? 00 48 8B 05 ? ? ? ? 0F 85 ? ? ? ? 48 8B 90 ? ? ? ? 48 85 D2 0F 84"; 
    inline constexpr const char* CookPathB    = "48 8B 0D ? ? ? ? E8 ? ? ? ? 48 89 C3 48 8B 0D ? ? ? ? E8 ? ? ? ? 48 89 C6";
    inline constexpr const char* CookFireWrite = "89 86 ? ? 00 00 89 8E ? ? 00 00 4C 89";
    inline constexpr const char* CookEntityVal = "48 85 DB 0F 84";
    inline constexpr const char* CookBplSkip   = "40 84 ED 75";
    inline constexpr const char* CookNullChk   = "48 85 C0 0F 84";
    inline constexpr const char* CookNullTgt1  = "48 8B 86 ? ? 00 00";
    inline constexpr const char* CookNullTgt2  = "48 85 DB";
    inline constexpr const char* CookShowPage  = "E8 ? ? ? ? 40 B6 01";
    inline constexpr const char* ExpHashCmp    = "81 F9 E1 73 90 69 0F 84"; 
    inline constexpr const char* ExpTailJmp    = "41 5F E9";
    inline constexpr const char* ExpTestJz     = "84 C0 0F 84";
    inline constexpr const char* CameraUpdateView = "56 48 83 EC 40 0F 29 7C 24 30 0F 29 74 24 20 48 89 CE F3 0F 10 71 70 F3 0F 10 79 78 F3 0F 5C F7 F3 0F 59 B1 80 00 00 00 E8 ?";
    inline constexpr const char* CameraStateBlenderTick = "41 57 41 56 41 55 41 54 56 57 55 53 B8 88 18 00 00 E8 ? ? ? ? 48 29 C4 44 0F 29 8C 24 70 18 00 00"; 
    inline constexpr const char* UnderwaterMaskPreMain = "41 56 56 57 55 53 48 81 EC F0 04 00 00";
    inline constexpr const char* UnderwaterMaskMain = "41 57 41 56 56 57 53 48 81 EC D0 04 00 00 48 89 CE";
    inline constexpr const char* UnderwaterMaskPostMain = "41 56 56 57 55 53 48 81 EC E0 00 00 00 48 89 CE 80 3D ? ? ? ? ? 75 ? 48 8B 86 ? ? ? ? 48 85 C0";
    inline constexpr const char* UnderwaterMaskClear = "56 57 48 83 EC 28 48 89 CE 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 48 8D BE ? ? ? ? 80 3D";
    inline constexpr const char* UpdateInnerTargetSig = "56 57 55 53 48 81 EC ? ? ? ? 44 0F 29 84 24 ? ? ? ? 0F 29 BC 24 ? ? ? ? 0F 29 B4 24 ? ? ? ? 48 89 CE 80 B9";
    
    inline constexpr const char* FreeCamCameraGetMain =
        "E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 "
        "E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 "
        "E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 "
        "48 8B 05 ? ? ? ? 48 8B 80 ? ? ? ? 48 85 C0 74 12";

    inline constexpr const char* FreeCamComponentGetTransform =
        "E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 "
        "E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 "
        "E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 56 48 83 EC 20 48 89 D6 E8 ? ? ? ?";

    inline constexpr const char* FreeCamTransformSetPosition =
        "E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 "
        "E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 "
        "56 48 83 EC 20 48 89 CE C7 41 ? ? ? ? 00 48 C7 01 00 00 00 00 48 89 D1 44 89 C2 49 89 F0 E8 ? ? ? ? "
        "48 89 F0 48 83 C4 20 5E C3 0F 1F 00 E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 56";

    inline constexpr const char* FreeCamTransformSetRotation =
        "E9 ? ? ? ? 66 66 2E 0F 1F 84 00 ? ? ? ? 56 48 83 EC 50 48 89 D0 48 89 CE 0F 57 C0 0F 29 44 24 30 "
        "48 8D 54 24 30 48 89 C1 E8 ? ? ? ? 0F 28 44 24 30 0F 29 44 24 40 0F 29 44 24 30 C7 44 24 28 00 00 00 00 "
        "48 C7 44 24 20 00 00 00 00 48 8D 4C 24 30 48 8D 54 24 20 E8 ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 10 4C 24 24 "
        "F3 0F 59 C8 F3 0F 10 54 24 20 F3 0F 59 D0 F3 0F 10 1D ? ? ? ? 0F 2E DA 76 ? F3 0F 10 25 ? ? ? ? EB ? "
        "0F 2E 15 ? ? ? ? 76 ? F3 0F 10 25 ? ? ? ? F3 0F 58 D4 F3 0F 59 44 24 28 0F 2E D9 76 ? F3 0F 10 25 ? ? ? ? EB ? "
        "0F 2E 0D ? ? ? ? 76 ? F3 0F 10 25 ? ? ? ? F3 0F 58 CC 0F 2E D8 76 ? F3 0F 10 1D ? ? ? ? EB ? "
        "0F 2E 05 ? ? ? ? 76 ? F3 0F 10 1D ? ? ? ? F3 0F 58 C3 F3 0F 11 16 F3 0F 11 4E 04 F3 0F 11 46 08 "
        "48 89 F0 48 83 C4 50 5E C3 0F 1F 40 00 56 48 83 EC 20 48 89 CE 0F 57 C0 0F 11 01 48 89 D1 48 89 F2 "
        "E8 ? ? ? ? 48 89 F0 48 83 C4 20 5E C3 66 66 66 66 66 2E 0F 1F 84 00 ? ? ? ? 56 48 83 EC 50 "
        "F3 0F 10 42 08 F3 0F 59 05 ? ? ? ? 48 89 CE F2 0F 10 0A 0F 59 0D ? ? ? ? 0F 13 4C 24 20 F3 0F 11 44 24 28 "
        "0F 57 C0 0F 29 44 24 30 48 8D 4C 24 20 48 8D 54 24 30 E8 ? ? ? ? 0F 28 44 24 30 0F 29 44 24 40 "
        "48 8D 54 24 40 48 89 F1 E8 ? ? ? ? 90 48 83 C4 50 5E C3 E9 ? ? ? ? 66 66 2E 0F 1F 84 00 ? ? ? ? 56 48 81 EC 80 00 00 00";

    inline constexpr const char* FreeCamTransformGetRotation =
        "56 48 83 EC 20 48 89 CE 0F 57 C0 0F 11 01 48 89 D1 48 89 F2 E8 ? ? ? ? "
        "48 89 F0 48 83 C4 20 5E C3 66 66 66 66 66 2E 0F 1F 84 00 ? ? ? ? 56 48 83 EC 50";

    inline constexpr const char* FreeCamTransformGetPosition =
        "56 48 83 EC 20 48 89 CE C7 41 ? ? ? ? 00 48 C7 01 00 00 00 00 48 89 D1 48 89 F2 E8 ? ? ? ? "
        "48 89 F0 48 83 C4 20 5E C3 66 0F 1F 44 00 00 E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 "
        "E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 "
        "E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 E9 ? ? ? ? 66 66 2E 0F 1F 84 ? ? ? ? 00 "
        "56 48 83 EC 20 48 89 CE C7 41 ? ? ? ? 00 48 C7 01 00 00 00 00 48 89 D1 44 89 C2";

    // 7.0 new
    namespace CN {
        inline constexpr const char* GetActiveOffset = ""; //does not exist
        inline constexpr const char* GetComponent = ""; //does not exist
        inline constexpr const char* GetText = "190A4300"; 
        inline constexpr const char* ClockPageOkOffset = "101193D0"; 
        inline constexpr const char* ClockPageCloseOffset = "EE03560"; 
        inline constexpr const char* ClockPageFinishOffset = "101181C0"; 
        inline constexpr const char* ClockPageBackOffset = "101151B0"; 
        inline constexpr const char* TouchInputOffset = "9DB42B0"; 
        inline constexpr const char* InnerDispatcherOffset = "E648A10"; 
        inline constexpr const char* EventCameraOffset = ""; //does not exist
        inline constexpr const char* DamageColorA = "1309AD50"; 
        inline constexpr const char* DamageColorB = "1309C460"; 
        inline constexpr const char* DamageColor1 = "1309C3F0"; 
        inline constexpr const char* DamageColor2 = "1309C2E0"; 
        inline constexpr const char* DamageColor3 = "1309C670";
        inline constexpr const char* DamageColor4 = "1309C270";
        inline constexpr const char* UpdateInnerTargetOffset = "6DE13F0";
        inline constexpr const char* SetActiveOffset = "13D8580";
        inline constexpr const char* ProfilePageRefreshOffset = "11B39290";
    }

    namespace OS {
        inline constexpr const char* GetActiveOffset = ""; //does not exist
        inline constexpr const char* GetComponent = ""; //does not exist
        inline constexpr const char* GetText = "19086640"; 
        inline constexpr const char* ClockPageOkOffset = "100FF760";
        inline constexpr const char* ClockPageCloseOffset = "EDEBCE0";
        inline constexpr const char* ClockPageFinishOffset = "100FDC70";
        inline constexpr const char* ClockPageBackOffset = "100FC930";
        inline constexpr const char* TouchInputOffset = "9D976C0";
        inline constexpr const char* InnerDispatcherOffset = "E291D10";
        inline constexpr const char* EventCameraOffset = "70D9930"; //special need 
        inline constexpr const char* DamageColorA = "1307CFB0";
        inline constexpr const char* DamageColorB = "1307C960";
        inline constexpr const char* DamageColor1 = "1307C8F0";
        inline constexpr const char* DamageColor2 = "1307CAB0";
        inline constexpr const char* DamageColor3 = "1307CD30";
        inline constexpr const char* DamageColor4 = "1307CA40";
        inline constexpr const char* UpdateInnerTargetOffset = "6DDF9E0";
        inline constexpr const char* SetActiveOffset = "13D8580";
    }

    /*  6.7 old
        namespace CN {
        inline constexpr const char* GetActiveOffset = ""; //does not exist
        inline constexpr const char* GetComponent = ""; //does not exist
        inline constexpr const char* GetText = "17DF06F0";
        inline constexpr const char* ClockPageOkOffset = "11919E90";
        inline constexpr const char* ClockPageCloseOffset = "ECF7DC0";
        inline constexpr const char* ClockPageFinishOffset = "11919A50";
        inline constexpr const char* ClockPageBackOffset = "119172C0";
        inline constexpr const char* TouchInputOffset = "8D00130";
        inline constexpr const char* InnerDispatcherOffset = "105C0E80";
        inline constexpr const char* EventCameraOffset = "0"; //does not exist
        inline constexpr const char* DamageColorA = "12426CE0";
        inline constexpr const char* DamageColorB = "124224F0";
        inline constexpr const char* DamageColor1 = "12423BE0";
        inline constexpr const char* DamageColor2 = "12422F40";
        inline constexpr const char* DamageColor3 = "12423B70";
        inline constexpr const char* DamageColor4 = "12422ED0";
        inline constexpr const char* UpdateInnerTargetOffset = "6812110";
    }

    namespace OS {
        inline constexpr const char* GetActiveOffset = ""; //does not exist
        inline constexpr const char* GetComponent = ""; //does not exist
        inline constexpr const char* GetText = "17E3EAE0";
        inline constexpr const char* ClockPageOkOffset = "11933430";
        inline constexpr const char* ClockPageCloseOffset = "ED0A3E0";
        inline constexpr const char* ClockPageFinishOffset = "119315A0";
        inline constexpr const char* ClockPageBackOffset = "11930990";
        inline constexpr const char* TouchInputOffset = "8CEAB90";
        inline constexpr const char* InnerDispatcherOffset = "105CAE70";
        inline constexpr const char* EventCameraOffset = "DEEA1B0"; //special need
        inline constexpr const char* DamageColorA = "12442850";
        inline constexpr const char* DamageColorB = "12442960";
        inline constexpr const char* DamageColor1 = "124426D0";
        inline constexpr const char* DamageColor2 = "12441000";
        inline constexpr const char* DamageColor3 = "12442660";
        inline constexpr const char* DamageColor4 = "12440F90";
    }
     */
}

namespace GameStrings {
    inline constexpr const char* SynthesisPage = "SynthesisPage";
    inline constexpr const char* QuestBannerPath = "Canvas/Pages/InLevelMapPage/GrpMap/GrpPointTips/Layout/QuestBanner";
    inline constexpr const char* ProfileLayerPath = "/Canvas/Pages/PlayerProfilePage";
    inline constexpr const char* UIDPathMain = "/Canvas/Pages/PlayerProfilePage/GrpProfile/Right/GrpPlayerCard/UID";
    inline constexpr const char* ProfileUIDPath = "/Canvas/Pages/PlayerProfilePage/GrpProfile/Right/GrpPlayerCard/UID";
    inline constexpr const char* ProfileBirthdayTargets[] = {
        "/Canvas/Pages/PlayerProfilePage/GrpProfile/Right/GrpPlayerCard/Birthday",
        "/Canvas/Pages/PlayerProfilePage/GrpProfile/Right/GrpPlayerCard/BirthDay",
        "/Canvas/Pages/PlayerProfilePage/GrpProfile/Right/GrpPlayerCard/BirthdaySlot",
        "/Canvas/Pages/PlayerProfilePage/GrpProfile/Right/GrpPlayerCard/GrpBirthday",
        "/Canvas/Pages/PlayerProfilePage/GrpProfile/Left/GrpPlayerInfo/Birthday",
        "/Canvas/Pages/PlayerProfilePage/GrpProfile/Left/GrpPlayerInfo/BirthdaySlot",
        "/Canvas/Pages/PlayerProfilePage/GrpProfile/Left/GrpBaseInfo/Birthday",
        "/Canvas/Pages/PlayerProfilePage/GrpProfile/Left/GrpBaseInfo/BirthdaySlot",
        "Birthday",
        "BirthDay",
        "BirthdaySlot",
        "Birthdate",
        "BirthDate",
        "GrpBirthday"
    };
    inline constexpr const char* UIDPathWatermark = "/BetaWatermarkCanvas(Clone)/Panel/TxtUID";

    inline constexpr const char* PaimonPath = "/EntityRoot/OtherGadgetRoot/NPC_Guide_Paimon(Clone)";
    inline constexpr const char* DivePaimonPath = "/EntityRoot/OtherGadgetRoot/NPC_Guide_Paimon_Dive(Clone)";
    inline constexpr const char* BeydPaimonPath = "/EntityRoot/OtherGadgetRoot/Beyd_NPC_Kanban_Paimon(Clone)";
}

namespace Offsets {
    extern std::string GetActiveOffset;
    extern std::string GetComponent;
    extern std::string GetText;
    extern std::string ClockPageOkOffset;
    extern std::string ClockPageCloseOffset;
    extern std::string ClockPageFinishOffset;
    extern std::string ClockPageBackOffset;
    extern std::string TouchInputOffset;
    extern std::string InnerDispatcherOffset;
    extern std::string EventCameraOffset;
    extern std::string DamageColorAOffset;
    extern std::string DamageColorBOffset;
    extern std::string DamageColor1Offset;
    extern std::string DamageColor2Offset;
    extern std::string DamageColor3Offset;
    extern std::string DamageColor4Offset;
    extern std::string UpdateInnerTargetOffset;
    extern std::string SetActiveOffset;
    extern std::string ProfilePageRefreshOffset;

    void InitOffsets(bool isOS);
}
