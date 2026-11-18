/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "AntiCensor.h"
#include "../Core/Compatibility.h"
#include "../MinHook/MinHook.h"
#include "../Patterns/Patterns.h"
#include <iostream>

extern "C"
{
    void AntiCensorCompareRegion();
    void* AntiCensorOriginalComparison = nullptr;
    void* AntiCensorSelectBundle = nullptr;
}

void AntiCensor::Init()
{
    using namespace Patterns::AntiCensor;
    using namespace Offsets::AntiCensor;

    const auto game = GetModuleHandleW(nullptr);
    if (GetModuleHandleW(L"YuanShen.exe") != game)
        return;

    auto* base = reinterpret_cast<uint8_t*>(game);
    // The detour depends on the native loop's register allocation.
    if (Compatibility::GetImageSize() != ImageSize ||
        !Compatibility::ResolveFunctionEntry(ReaderRva, ReaderEntry, ReaderCheckSize) ||
        Scanner::ScanUniqueMainMod(HashRead, base + HashReadRva, HashReadCheckSize) != base + HashReadRva ||
        Scanner::ScanUniqueMainMod(RegionDecision, base + DecisionStartRva, DecisionCheckSize) != base + DecisionStartRva)
    {
        std::cout << "[WARN] AntiCensor: unsupported CN asset index reader.\n";
        return;
    }

    auto* comparison = base + CompareRva;
    AntiCensorSelectBundle = base + SelectBundleRva;
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
