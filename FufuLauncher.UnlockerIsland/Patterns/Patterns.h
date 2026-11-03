/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once
#include <cstdint>
#include <string>

namespace Patterns
{
    inline constexpr const char* GetFrameCount = "E8 ? ? ? ? 85 C0 7E 0E E8 ? ? ? ? 0F 57 C0 F3 0F 2A C0 EB 08";
    inline constexpr const char* SetFrameCount = "E8 ? ? ? ? E8 ? ? ? ? 83 F8 1F 0F 9C 05 ? ? ? ? 48 8B 05";
    inline constexpr const char* ChangeFOV =
        "40 53 48 83 EC 60 0F 29 74 24 ? 48 8B D9 0F 28 F1 E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? E8 ? ? ? ? 48 8B C8";
    inline constexpr const char* SwitchInputDeviceToTouchScreen =
        "56 57 48 83 EC ? 48 89 CE 80 3D ? ? ? ? 00 48 8B 05 ? ? ? ? 0F 85 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9 0F 84 ? ? ? ? 48 8B 15 ? ? ? ? E8 ? ? ? ? 48 89 C7 48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9 0F 84 ? ? ? ? 31 D2";
    inline constexpr const char* QuestBanner =
        "41 57 41 56 56 57 55 53 48 81 EC E8 00 00 00 0F 29 BC 24 ? ? ? ? 0F 29 B4 24 ? ? ? ? 48 89 CE 0F 57 C0 0F 29 84 24 ? ? ? ? 0F 29 84 24 ? ? ? ? 0F 29 84 24";
    inline constexpr const char* FindGameObject =
        "40 53 48 83 EC ? 48 89 4C 24 ? 48 8D 54 24 ? 48 8D 4C 24 ? E8 ? ? ? ? 48 8B 08 48 85 C9 75 ? 48 8D 48 ? E8 ? ? ? ? 48 8B 4C 24 ? 48 8B D8 48 85 C9 74 ? 48 83 7C 24 ? 00 76";
    inline constexpr const char* SetActive = ""; //removed, use offset
    inline constexpr const char* DamageText =
        "41 57 41 56 41 55 41 54 56 57 55 53 48 81 EC 28 02 00 00 44 0F 29 BC 24 ? ? ? ? 44 0F 29 B4 24 ? ? ? ? 44 0F 29 AC 24 ? ? ? ? 44 0F 29 A4 24 ? ? ? ? 44 0F 29 9C 24 ? ? ? ? 44 0F 29 94 24 ? ? ? ? 44 0F 29 8C 24 ? ? ? ? 44 0F 29 84 24 ? ? ? ? 0F 29 BC 24 ? ? ? ? 0F 29 B4 24 ? ? ? ? 45 89 CD 89 D5 48 89 CE F3 44 0F 10 9C 24 ? ? ? ?";
    inline constexpr const char* EventCamera =
        "41 57 41 56 56 57 55 53 48 83 EC 48 48 89 D7 49 89 CE 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 80";
    inline constexpr const char* FindString =
        "56 48 83 ec 20 48 89 ce e8 ? ? ? ? 48 89 f1 89 c2 48 83 c4 20 5e e9 ? ? ? ? cc cc cc cc";
    inline constexpr const char* CraftPartner =
        "41 57 41 56 41 55 41 54 56 57 55 53 48 81 EC ? ? ? ? 4D 89 ? 4C 89 C6 49 89 D4 49 89 CE";
    inline constexpr const char* CraftEntry =
        "41 56 56 57 53 48 83 EC 48 49 89 CE 80 3D ? ? ? ? 00 0F 84 ? ? ? ? 80 3D ? ? ? ? 00 48 8B 0D ? ? ? ? 0F 85 ? ? ? ? 48 8B 81 ? ? ? ? 48 85 C0 0F 84";
    inline constexpr const char* CheckCanEnter =
        "56 48 81 ec 80 00 00 00 80 3d ? ? ? ? 00 0f 84 ? ? ? ? 80 3d ? ? ? ? 00";
    inline constexpr const char* OpenTeamPage =
        "56 57 53 48 83 ec 20 89 cb 80 3d ? ? ? ? 00 74 7a 80 3d ? ? ? ? 00 48 8b 05";
    inline constexpr const char* OpenTeam =
        "48 83 EC 28 80 3D ? ? ? ? 00 75 ? 48 8B 0D ? ? ? ? 80 B9 C7 00 00 00 00 74 ? B9 0D 00 00 00 E8 ? ? ? ? 84 C0 74";
    inline constexpr const char* DisplayFog =
        "0F B6 02 88 01 8B 42 04 89 41 04 F3 0F 10 52 ? F3 0F 10 4A ? F3 0F 10 42 ? 8B 42 08";
    inline constexpr const char* PlayerPerspective =
        "E8 ? ? ? ? 48 8B BE ? ? ? ? 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 80 BE ? ? ? ? ? 74 11";
    inline constexpr const char* SetSyncCount =
        "E8 ? ? ? ? E8 ? ? ? ? 89 C6 E8 ? ? ? ? 31 C9 89 F2 49 89 C0 E8 ? ? ? ? 48 89 C6 48 8B 0D ? ? ? ? 80 B9 ? ? ? ? ? 74 47 48 8B 3D ? ? ? ? 48 85 DF 74 4C";
    inline constexpr const char* GameUpdate =
        "55 56 57 53 48 83 EC ? 48 8D 6C 24 ? 48 C7 45 ? ? ? ? ? 48 8B 41 ? 48 85 C0 0F 84 ? ? ? ? 83 78";
    inline constexpr const char* CheckCanOpenMap =
        "E8 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? ?? ?? 48 8B 45 ?? 48 85 C0 74 ?? 41 8B 17 4C 8B 40 ?? 48 8B 48 ?? FF 50 ?? 84 C0 0F 84 ?? ?? ?? ??";
    inline constexpr const char* SetupResinList =
        "E8 ? ? ? ? 84 DB 74 ? 4C 89 F1 E8 ? ? ? ? 49 8B 86 ? ? ? ? 48 85 C0 75 ? E9 ? ? ? ?";
    inline constexpr const char* RefreshResinList =
        "41 56 56 57 53 48 83 EC 28 89 D3 49 89 CE 80 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 49 8B 86 ?? ?? ?? ?? 48 85 C0 0F 84 ?? ?? ?? ?? 48 83 78 10 00 0F 84 ?? ?? ?? ?? 4C 89 F1 E8 ?? ?? ?? ?? 84 DB";
    inline constexpr const char* ResinSetupView =
        "56 48 83 EC 20 48 89 CE 80 3D ?? ?? ?? ?? 00 74 ?? 80 3D ?? ?? ?? ?? 00 75 ?? 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 48 8B 8E 88 00 00 00 48 85 C9 0F 84 ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 89 86 40 02 00 00 48 85 C0 74 ?? 48 83 78 10 00 74 ?? 48 89 F1 B2 01 48 83 C4 20 5E E9 ?? ?? ?? ??";
    inline constexpr const char* ResinContextShow =
        "56 53 48 83 EC 28 89 D3 48 89 CE 80 3D ?? ?? ?? ?? 00 75 ?? 84 DB 74 ?? 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 48 8B 8E ?? ?? ?? ?? 48 85 C9 74 ?? B2 01 E8 ?? ?? ?? ??";
    inline constexpr const char* ResinInventoryChanged =
        "56 57 48 81 EC 88 00 00 00 41 89 D0 48 89 CA 80 3D ?? ?? ?? ?? 00 48 8B 05 ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 48 8B B0 50 9E 00 00 48 8B 42 48 48 C7 44 24 48 00 00 00 00 48 89 44 24 40 44 89 44 24 48 48 8B 0D ?? ?? ?? ?? 48 8D 54 24 40 E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 0F 57 C0 0F 29 44 24 20 48 C7 44 24 30 00 00 00 00 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? C7 44 24 20 1B 01 00 00";
    inline constexpr const char* ResinItemCount =
        "48 8B 05 ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 48 8B 88 ?? ?? ?? ?? 48 85 C9 0F 84 ?? ?? ?? ?? BA 04 A2 01 00 E8 ?? ?? ?? ?? 48 85 C0 0F 8E";
    inline constexpr const char* ResinContextDisable =
        "48 83 EC 28 48 89 CA 80 3D ?? ?? ?? ?? 00 75 ?? 48 89 D1 48 83 C4 28 E9 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 8B 80 ?? ?? ?? ?? 48 8B 88 ?? ?? ?? ?? 48 85 C9 74 ?? 48 83 C4 28 E9 ?? ?? ?? ??";
    inline constexpr const char* GetName =
        "40 53 48 81 EC ?? ?? ?? ?? 48 8B D9 48 85 C9 0F 84 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0 0F 84 ?? ?? ?? ?? 48 8B 10 48 8B C8 FF 52 ?? 48 85 C0 0F 85 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ??";
    inline constexpr const char* GetActive =
        "E8 ?? ?? ?? ?? 84 C0 74 ?? 48 89 F1 E8 ?? ?? ?? ?? 48 8B 4E ?? 48 85 C9 0F 84 ?? ?? ?? ?? 80 79 ?? ?? 0F 94 C1 08 C1";
    inline constexpr const char* AvatarPaimonAppear =
        "41 56 56 57 53 48 83 EC 28 44 89 C3 48 89 D6 49 89 CE 80 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 80 B9 C7 00 00 00 00 0F 84 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0 0F 84 ?? ?? ?? ??";
    inline constexpr const char* StringNew = "56 48 83 EC 20 48 85 C9 74 ? 48 89 CE E8 ? ? ? ? 48 89 F1 89 C2";
    inline constexpr const char* ShowDialog = "41 57 41 56 56 57 55 53 48 83 EC 28 4D 89 CF 4C 89 C7 48 89 D5 48 89 CB";
    inline constexpr const char* CookHandler =
        "41 56 56 57 55 53 48 83 EC 20 48 89 D3 49 89 CE 80 3D ? ? ? ? 00 0F 84 ? ? ? ? 80 3D ? ? ? ? 00 48 8B 05 ? ? ? ? 0F 85 ? ? ? ? 48 8B 90 ? ? ? ? 48 85 D2 0F 84";
    inline constexpr const char* CookPathB =
        "48 8B 0D ? ? ? ? E8 ? ? ? ? 48 89 C3 48 8B 0D ? ? ? ? E8 ? ? ? ? 48 89 C6";
    inline constexpr const char* CookFireWrite = "89 86 ? ? 00 00 89 8E ? ? 00 00 4C 89";
    inline constexpr const char* CookEntityVal = "48 85 DB 0F 84";
    inline constexpr const char* CookBplSkip = "40 84 ED 75";
    inline constexpr const char* CookNullChk = "48 85 C0 0F 84";
    inline constexpr const char* CookNullTgt1 = "48 8B 86 ? ? 00 00";
    inline constexpr const char* CookNullTgt2 = "48 85 DB";
    inline constexpr const char* CookShowPage = "E8 ? ? ? ? 40 B6 01";
    inline constexpr const char* ExpHashCmp = "81 F9 E1 73 90 69";
    inline constexpr const char* ExpTailJmp = "41 5F E9";
    inline constexpr const char* ExpTestJz = "84 C0 0F 84";
    inline constexpr const char* CameraFollowTick =
        "56 57 53 48 81 EC 90 00 00 00 44 0F 29 84 24 ? ? ? ? 0F 29 7C 24 ? 0F 29 74 24 ? 4C 89 CB";
    inline constexpr const char* CameraAngularDecay =
        "56 48 81 EC 80 00 00 00 44 0F 29 44 24 ? 0F 29 7C 24 ? 0F 29 74 24 ? 0F 28 FB 44 0F 28 C2 48 89 D6 0F 28 F0";
    inline constexpr const char* CameraInputDevice =
        "48 8B 80 C0 9E 00 00 48 85 C0 0F 84 ? ? ? ? 83 B8 AC 01 00 00 03 48 8B 47 18 75";
    inline constexpr const char* CameraStateBlenderTick =
        "41 57 41 56 41 55 41 54 56 57 55 53 B8 88 18 00 00 E8 ? ? ? ? 48 29 C4 44 0F 29 8C 24 70 18 00 00";
    inline constexpr const char* CameraCollisionTick =
        "41 57 41 56 56 57 53 48 81 EC 20 03 00 00 66 44 0F 29 BC 24 10 03 00 00 66 44 0F 29 B4 24 00 03 00 00";
    inline constexpr const char* CameraDistanceCollect =
        "41 57 41 56 41 54 56 57 53 48 83 EC 48 0F 29 74 24 30 4D 89 CC 4C 89 C6 66 0F 28 F1 48 89 CF 4C 8B B4 24 A0 00 00 00 80 3D ? ? ? ? 00 0F 85 ? ? ? ? 48 89 F9 48 89 F2 4D 89 F0 E8 ? ? ? ? 48 89 F9 66 0F 28 CE";
    inline constexpr const char* CameraDistanceInitializeRadius =
        "41 56 56 57 55 53 48 81 EC C0 00 00 00 44 0F 29 8C 24 ? ? ? ? 44 0F 29 84 24 ? ? ? ? 0F 29 BC 24 ? ? ? ? 66 0F 29 B4 24 ? ? ? ? 4C 89 CB 4C 89 C6 66 44 0F 28 C1 49 89 CE 0F 57 C0 0F 29 44 24 30 80 3D ? ? ? ? 00 0F 85 ? ? ? ?";
    inline constexpr const char* CameraDistanceIsRadiusMode =
        "56 57 48 83 EC 38 89 D7 80 3D ? ? ? ? 00 75 ? 8B 01 8B 51 04 8B 49 08 89 C6 09 D6 75 ? 85 C9 75 ? EB ? 81 C2 92 ED EE 50 39 D0 75 ? 81 F1 D3 33 26 58 01 C8 05 8C 00 36 CA EB ? 80 3D ? ? ? ? 00 48 8B 05 ? ? ? ? 75 ? 48 8B 88 E0 BE 00 00 48 85 C9 74 ? BA 03 00 00 00 E8 ? ? ? ? 31 C0 39 F8 0F 94 C0 48 83 C4 38 5F 5E C3";
    inline constexpr const char* CameraDistanceIsRadiusModeOS =
        "56 57 48 83 EC 38 89 D7 80 3D ? ? ? ? 00 75 ? 8B 01 8B 51 04 8B 49 08 89 C6 09 D6 75 ? 85 C9 75 ? EB ? 81 C2 92 ED EE 50 39 D0 75 ? 81 F1 D3 33 26 58 01 C8 05 8C 00 36 CA EB ? 80 3D ? ? ? ? 00 48 8B 05 ? ? ? ? 75 ? 48 8B 88 60 BE 00 00 48 85 C9 74 ? BA 03 00 00 00 E8 ? ? ? ? 31 C0 39 F8 0F 94 C0 48 83 C4 38 5F 5E C3";
    inline constexpr const char* UnderwaterMaskPreMain = "41 56 56 57 55 53 48 81 EC F0 04 00 00";
    inline constexpr const char* UnderwaterMaskMain = "41 57 41 56 56 57 53 48 81 EC D0 04 00 00 48 89 CE";
    inline constexpr const char* UnderwaterMaskPostMain =
        "41 56 56 57 55 53 48 81 EC E0 00 00 00 48 89 CE 80 3D ? ? ? ? ? 75 ? 48 8B 86 ? ? ? ? 48 85 C0";
    inline constexpr const char* UnderwaterMaskClear =
        "56 57 48 83 EC 28 48 89 CE 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 48 8D BE ? ? ? ? 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 8B 17 85 D2 78";
    inline constexpr const char* UpdateInnerTargetSig =
        "56 57 55 53 48 81 EC ? ? ? ? 44 0F 29 84 24 ? ? ? ? 0F 29 BC 24 ? ? ? ? 0F 29 B4 24 ? ? ? ? 48 89 CE 80 B9";
    inline constexpr const char* SetupPlayerProfilePage =
        "55 41 57 41 56 41 55 41 54 56 57 53 48 81 EC ?? ?? ?? ?? 48 8D AC 24 ?? ?? ?? ?? 0F 29 75 ?? 48 C7 45 ?? ?? ?? ?? ?? 49 89 CC 80 3D ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 0F 85 ?? ?? ?? ??";


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
    
    inline constexpr const char* CameraInputDeviceOS =
        "48 8B 80 80 A0 00 00 48 85 C0 0F 84 ?? ?? ?? ?? 83 B8 58 01 00 00 03 48 8B 47 10 75";
    inline constexpr const char* ResinSetupViewOS =
        "56 48 83 EC 20 48 89 CE 80 3D ?? ?? ?? ?? 00 74 ?? 80 3D ?? ?? ?? ?? 00 75 ?? "
        "80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 48 8B 4E 10 48 85 C9 0F 84 ?? ?? ?? ?? "
        "48 8B 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 89 86 28 02 00 00 48 85 C0 74 ?? "
        "48 83 78 10 00 74 ?? 48 89 F1 B2 01 48 83 C4 20 5E E9 ?? ?? ?? ??";
    inline constexpr const char* ResinInventoryChangedOS =
        "56 57 48 81 EC 88 00 00 00 41 89 D0 48 89 CA 80 3D ?? ?? ?? ?? 00 48 8B 05 ?? ?? ?? ?? "
        "0F 85 ?? ?? ?? ?? 48 8B B0 70 9E 00 00 48 8B 42 60 48 C7 44 24 48 00 00 00 00 "
        "48 89 44 24 40 44 89 44 24 48 48 8B 0D ?? ?? ?? ?? 48 8D 54 24 40 E8 ?? ?? ?? ?? "
        "80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 0F 57 C0 0F 29 44 24 20 48 C7 44 24 30 00 00 00 00 "
        "80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? C7 44 24 20 1B 01 00 00";

    namespace CN
    {
        inline constexpr const char* GetActiveOffset = ""; //does not exist
        inline constexpr const char* GetComponent = ""; //does not exist
        inline constexpr const char* GetText = "1A30DD80";
        // 48 8B 81 E0 00 00 00 C3 0F 1F 84 00 00 00 00 00 56 57 48 83 EC 28 48 89 CE 48 85 D2 0F 84 C2 00 00 00 48 89 D7 8B 42 10
        inline constexpr const char* ClockPageOkOffset = "AE127B0";
        // 56 57 55 53 48 83 EC 28 48 89 CE 80 3D ? ? ? ? 00 0F 85 ? ? ? ? 80 BE 4A 02 00 00 00 74 ? 48 83 C4 28 5B 5D 5F
        inline constexpr const char* ClockPageCloseOffset = "9AAA620";
        // 56 57 53 48 83 EC 20 48 89 CE 80 3D ? ? ? ? 00 0F 85 ? ? ? ? 48 8B 8E 10 02 00 00 48 85 C9 0F 84 ? ? ? ? 83
        inline constexpr const char* ClockPageFinishOffset = "AE0E270";
        // 56 48 83 EC 30 48 8B 05 ? ? ? ? 48 8B 90 E0 8E 01 00 48 85 D2 0F 84 ? ? ? ? 48 89 CE 80 3D ? ? ? ? 00 0F 85
        inline constexpr const char* ClockPageBackOffset = "AE0F930";
        // 56 57 48 83 EC 38 0F 29 74 24 20 48 89 CE 80 3D ? ? ? ? 00 0F 84 ? ? ? ? 80 3D ? ? ? ? 00 0F 85 ? ? ? ?
        inline constexpr const char* TouchInputOffset = "D484110";
        // 56 57 48 83 EC 28 48 89 CE 80 3D ? ? ? ? 00 48 8B 05 ? ? ? ? 0F 85 ? ? ? ? 48 8B 88 60 9E 00 00 48 85 C9 0F
        inline constexpr const char* InnerDispatcherOffset = "10EF9DF0";
        // 41 57 41 56 41 55 41 54 56 57 55 53 48 81 EC 08 01 00 00 4C 89 CD 4C 89 C7 48 89 D3 48 89 CA 4C 8B A4 24 80 01 00 00 44
        inline constexpr const char* EventCameraOffset = ""; //does not exist
        inline constexpr const char* DamageColorA = "13F4C370";
        // 56 57 53 48 83 EC 40 44 89 CF 4C 89 C3 48 89 CE 80 3D ? ? ? ? 00 75 ? 48 8B 0D ? ? ? ? 80 B9 C7 00 00 00 00 74
        inline constexpr const char* DamageColorB = "13F4D2E0";
        // 56 57 53 48 83 EC 40 44 89 CB 4C 89 C7 48 89 CE 80 3D ? ? ? ? 00 75 ? 48 8B 0D ? ? ? ? 80 B9 C7 00 00 00 00 74
        inline constexpr const char* DamageColor1 = "13F4C0C0";
        // 56 48 83 EC 30 45 89 C1 48 89 D0 48 89 CE 80 3D ? ? ? ? 00 75 ? 4C 8B 40 60 48 8D 4C 24 20 48 89 C2 E8 ? ? ? ?
        inline constexpr const char* DamageColor2 = "13F4DBD0";
        // 56 48 83 EC 30 45 89 C1 48 89 D0 48 89 CE 80 3D ? ? ? ? 00 75 ? 4C 8B 40 48 48 8D 4C 24 20 48 89 C2 E8 ? ? ? ?
        inline constexpr const char* DamageColor3 = "13F4C050";
        // 56 48 83 EC 30 45 89 C1 48 89 D0 48 89 CE 80 3D ? ? ? ? 00 75 ? 4C 8B 40 58 48 8D 4C 24 20 48 89 C2 E8 ? ? ? ?
        inline constexpr const char* DamageColor4 = "13F4C300";
        // 56 48 83 EC 30 45 89 C1 48 89 D0 48 89 CE 80 3D ? ? ? ? 00 75 ? 4C 8B 40 40 48 8D 4C 24 20 48 89 C2 E8 ? ? ? ?
        inline constexpr const char* UpdateInnerTargetOffset = "71B3400";
        // 56 57 55 53 48 81 EC C8 00 00 00 44 0F 29 84 24 B0 00 00 00 0F 29 BC 24 A0 00 00 00 0F 29 B4 24 90 00 00 00 48 89 CE 80
        inline constexpr const char* SetActiveOffset = "1463FC0";
        // 48 89 5C 24 08 57 48 83 EC 20 0F B6 FA 48 8B D9 48 85 C9 74 ? E8 ? ? ? ? 48 85 C0 74 ? 40 84 FF 48 8B C8 0F 95 C2
    }


    namespace DomainMap
    {
        inline constexpr uint32_t TimeDateStamp = 0x6AD886FB;
        inline constexpr uint32_t SizeOfImage = 0x1B65F000;
        inline constexpr uint32_t CheckSum = 0x1AE0CBAC;

        inline constexpr uint32_t GuardCallInSource = 0x2D;
        inline constexpr uint32_t DailyCallA = 0x1BD;
        inline constexpr uint32_t DailyCallB = 0x262;
        inline constexpr uint32_t DailyCallReturnBias = 0xA;
        inline constexpr uint32_t TypeToModules = 0x10;
        inline constexpr uint32_t GuardToModules = 0x1C;

        inline constexpr uint32_t ModuleSceneSlot = 0xB330;
        inline constexpr uint32_t ModuleTransitionSlot = 0xBE90;
        inline constexpr uint32_t ModuleDungeonSlot = 0xA400;
        inline constexpr uint32_t SceneRootToScene = 0x60;
        inline constexpr uint32_t SceneToScript = 0x28;
        inline constexpr uint32_t ScriptState = 0x70;
        inline constexpr uint32_t SceneState = 0x64;
        inline constexpr uint32_t TransitionRootToObject = 0x30;
        inline constexpr uint32_t TransitionState = 0x38;
        inline constexpr uint32_t TransitionDungeonId = 0x28;
        inline constexpr uint32_t DungeonId = 0x2CC;

        inline constexpr const char* SourceEntry =
            "41 56 56 57 55 53 48 83 EC 20 41 89 D6 48 89 CF "
            "80 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? "
            "48 89 F9 E8 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? "
            "48 8B 88 00 A4 00 00 48 85 C9 0F 84 ?? ?? ?? ?? 44 89 F2 E8 ?? ?? ?? ??";
        inline constexpr const char* SourceGuard =
            "56 48 83 EC 30 48 89 CE 80 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? "
            "80 3D ?? ?? ?? ?? 00 48 8B 05 ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? "
            "48 8B 88 80 9E 00 00 48 85 C9 0F 84 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 75 26";
        inline constexpr const char* DungeonTypeCheck =
            "56 57 53 48 83 EC 20 89 CE 80 3D ?? ?? ?? ?? 00 48 8B 05 ?? ?? ?? ?? "
            "75 61 48 83 B8 80 9E 00 00 00 74 78 48 8B 88 90 BE 00 00 "
            "48 85 C9 74 71 E8 ?? ?? ?? ?? 48 85 C0 74 6C 8B 78 28";
        inline constexpr const char* DailyFightCall = "B9 03 00 00 00 E8 ?? ?? ?? ?? 84 C0";
    }

    
    namespace OS
    {
        inline constexpr const char* GetActiveOffset = ""; //does not exist
        inline constexpr const char* GetComponent = ""; //does not exist
        inline constexpr const char* GetText = "1A33FE50";
        // 48 8B 81 E0 00 00 00 C3 0F 1F 84 00 ? ? ? ? 56 57 48 83 EC 28 48 89 CE 48 85 D2 0F 84
        inline constexpr const char* ClockPageOkOffset = "AE164A0";
        // 56 57 55 53 48 83 EC 28 48 89 CE 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 80 BE ? ? ? ? ? 74 ? 48 83 C4 28 5B 5D 5F 5E C3 48 8B 86
        inline constexpr const char* ClockPageCloseOffset = "9AAE250";
        // 56 57 53 48 83 EC 20 48 89 CE 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 48 8B 8E ? ? ? ? 48 85 C9 0F 84
        inline constexpr const char* ClockPageFinishOffset = "AE169F0";
        // 56 48 83 EC 30 48 8B 05 ? ? ? ? 48 8B 90 60 8E 01 00 48 85 D2 0F 84 ? ? ? ? 48 89 CE 80 3D
        inline constexpr const char* ClockPageBackOffset = "AE15410";
        // 56 57 48 83 EC 38 0F 29 74 24 ? 48 89 CE 80 3D ? ? ? ? ? 0F 84 ? ? ? ? 80 3D ? ? ? ? ? 0F 85 ? ? ? ? C6 86 ? ? ? ? 01
        inline constexpr const char* TouchInputOffset = "D49A5B0";
        // 56 57 48 83 EC 28 48 89 CE 80 3D ? ? ? ? ? 48 8B 05 ? ? ? ? 0F 85 ? ? ? ? 48 8B 88 80 9E 00 00 48 85 C9 0F 84
        inline constexpr const char* InnerDispatcherOffset = "10EF6610";
        // 41 57 41 56 41 55 41 54 56 57 55 53 48 81 EC 08 01 00 00 4C 89 CD 4C 89 C7 48 89 D3 48 89 CA
        inline constexpr const char* EventCameraOffset = "C5438F0"; //special need
        // 41 57 41 56 56 57 55 53 48 83 EC 48 48 89 D7 49 89 CE 80 3D ? ? ? ? 00 0F 85 ? ? ? ? 80
        inline constexpr const char* DamageColorA = "13F58A60";
        // 56 57 53 48 83 EC 40 44 89 CF 4C 89 C3 48 89 CE 80 3D ? ? ? ? ? 75 ? 48 8B 0D ? ? ? ? 80 B9 C7 00 00 00 00 74 ? B9 35 0E 00 00 E8
        inline constexpr const char* DamageColorB = "13F57D00";
        // 56 57 53 48 83 EC 40 44 89 CB 4C 89 C7 48 89 CE 80 3D ? ? ? ? ? 75 ? 48 8B 0D ? ? ? ? 80 B9 C7 00 00 00 00 74 ? B9 35 0E 00 00 E8
        inline constexpr const char* DamageColor1 = "13F5A340";
        // 56 48 83 EC 30 45 89 C1 48 89 D0 48 89 CE 80 3D ? ? ? ? ? 75 ? 4C 8B 40 60 48 8D 4C 24 ? 48 89 C2
        inline constexpr const char* DamageColor2 = "13F5A5C0";
        // 56 48 83 EC 30 45 89 C1 48 89 D0 48 89 CE 80 3D ? ? ? ? ? 75 ? 4C 8B 40 48 48 8D 4C 24 ? 48 89 C2
        inline constexpr const char* DamageColor3 = "13F57C90";
        // 56 48 83 EC 30 45 89 C1 48 89 D0 48 89 CE 80 3D ? ? ? ? ? 75 ? 4C 8B 40 58 48 8D 4C 24 ? 48 89 C2
        inline constexpr const char* DamageColor4 = "13F589F0";
        // 56 48 83 EC 30 45 89 C1 48 89 D0 48 89 CE 80 3D ? ? ? ? ? 75 ? 4C 8B 40 40 48 8D 4C 24 ? 48 89 C2
        inline constexpr const char* UpdateInnerTargetOffset = "71B2E20";
        // 56 57 55 53 48 81 EC C8 00 00 00 44 0F 29 84 24 ? ? ? ? 0F 29 BC 24 ? ? ? ? 0F 29 B4 24 ? ? ? ?
        inline constexpr const char* SetActiveOffset = "1464FC0";
        // 48 89 5C 24 08 57 48 83 EC 20 0F B6 FA 48 8B D9 48 85 C9 74 ? E8 ? ? ? ? 48 85 C0 74 ? 40 84 FF 48 8B C8 0F 95 C2
    }
}

namespace GameStrings
{
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

namespace Offsets
{
    extern std::string GetActiveOffset;
    extern std::string GetComponent;
    extern std::string GetText;
    extern bool IsOS;
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

    void InitOffsets(bool isOS);
}
