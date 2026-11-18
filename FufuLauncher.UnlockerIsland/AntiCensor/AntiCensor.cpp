/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "AntiCensor.h"
#include "../Core/Compatibility.h"
#include "../MinHook/MinHook.h"
#include <iostream>

extern "C"
{
    void AntiCensorCompareRegion();
    void* AntiCensorOriginalComparison = nullptr;
    void* AntiCensorSelectBundle = nullptr;
}

void AntiCensor::Init()
{
    constexpr uintptr_t readerRva = 0x06F00580;
    constexpr uintptr_t hashReadRva = 0x06F0068F;
    constexpr uintptr_t compareRva = 0x06F0071C;
    constexpr uintptr_t selectBundleRva = 0x06F00722;
    constexpr uintptr_t decisionStartRva = 0x06F0070E;
    constexpr char readerSignature[] =
        "41 57 41 56 41 55 41 54 56 57 55 53 48 83 EC 38 48 89 D6 49 89 CF 80 3D 70 8E 8C FE 00 75 3E";
    constexpr char hashReadSignature[] =
        "FF 90 60 01 00 00 49 89 C5";
    constexpr char decisionSignature[] =
        "FF 90 48 01 00 00 0F B6 CB 85 FF 0F 44 F8 39 0D DE C9 96 FE 0F 44 F8 41 FF CE 41 83 FE 01 77 A2 E9 BF 00 00 00";

    const auto game = GetModuleHandleW(nullptr);
    if (GetModuleHandleW(L"YuanShen.exe") != game)
        return;

    auto* base = reinterpret_cast<uint8_t*>(game);
    // The detour depends on the native loop's register allocation.
    if (Compatibility::GetImageSize() != 0x1B65F000 ||
        !Compatibility::ResolveFunctionEntry(readerRva, readerSignature, 31) ||
        Scanner::ScanUniqueMainMod(hashReadSignature, base + hashReadRva, 9) != base + hashReadRva ||
        Scanner::ScanUniqueMainMod(decisionSignature, base + decisionStartRva, 37) != base + decisionStartRva)
    {
        std::cout << "[WARN] AntiCensor: unsupported CN asset index reader.\n";
        return;
    }

    auto* comparison = base + compareRva;
    AntiCensorSelectBundle = base + selectBundleRva;
    if (MH_CreateHook(comparison, reinterpret_cast<void*>(&AntiCensorCompareRegion),
                      &AntiCensorOriginalComparison) != MH_OK)
    {
        std::cout << "[WARN] AntiCensor: failed to create the asset region hook.\n";
        return;
    }
    if (MH_EnableHook(comparison) != MH_OK)
    {
        MH_RemoveHook(comparison);
        std::cout << "[WARN] AntiCensor: failed to enable the asset region hook.\n";
        return;
    }
    std::cout << "[SCAN] AntiCensor asset region hook enabled.\n";
}
