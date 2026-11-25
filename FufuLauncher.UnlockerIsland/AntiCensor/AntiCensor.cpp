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

namespace
{
    struct StoryCostume
    {
        uint64_t alternatePath;
        uint64_t originalPath;
    };

    // GameObject path hashes of the CostumeWic NPC prefabs and their original counterparts.
    constexpr StoryCostume storyCostumes[] = {
        {0x1E91FA4EA48DB9D3ULL, 0xAD3D81DEF51B4749ULL}, // Qin
        {0xC7FBE4A6386B2C69ULL, 0x2BB46CB9359AB870ULL}, // Qin_Edit
        {0x28B9BDBA44299875ULL, 0x7E999FDC86B491EEULL}, // Ambor
        {0xAECE0AEBB5773524ULL, 0xD5CEFFD774380F19ULL}, // Ambor_Edit
        {0xB46BB98D8E58A4ABULL, 0xD5F630DDB3E67B91ULL}, // Mona
        {0xA2B19315F1310CD0ULL, 0xD0E5D223982333ADULL}, // Mona_Edit
        {0x351CB3BE114A99E1ULL, 0x41F28DF079507D61ULL}, // Rosaria
        {0x3D6E90F61DA6D6D3ULL, 0xD1625B3345113A22ULL}, // Rosaria_Edit
        {0x126BC0C034BC3EF1ULL, 0x5D75F13AFB5A7CFAULL}, // PhantomQin_10241_Edit
    };

    using ResourceIdentifierLookup = uint64_t (*)(void*, uint64_t, const void*);
    ResourceIdentifierLookup originalResourceIdentifier = nullptr;

    uint64_t ResolveStoryCostume(void* index, uint64_t assetPath, const void* method)
    {
        for (const auto& costume : storyCostumes)
        {
            if (assetPath != costume.alternatePath)
                continue;

            // The lower 32 bits are the bundle ID. Keep the asset name and
            // bundle ID from the same native lookup.
            const auto original = originalResourceIdentifier(index, costume.originalPath, method);
            if (static_cast<uint32_t>(original) != 0)
                return original;
            break;
        }
        return originalResourceIdentifier(index, assetPath, method);
    }

    bool InitRegionalAssets(uint8_t* base)
    {
        using namespace Patterns::AntiCensor;
        using namespace Offsets::AntiCensor;

        // The detour depends on the native loop's register allocation.
        if (Compatibility::GetImageSize() != ImageSize ||
            !Compatibility::ResolveFunctionEntry(ReaderRva, ReaderEntry, ReaderCheckSize) ||
            Scanner::ScanUniqueMainMod(HashRead, base + HashReadRva, HashReadCheckSize) != base + HashReadRva ||
            Scanner::ScanUniqueMainMod(RegionDecision, base + DecisionStartRva, DecisionCheckSize) != base + DecisionStartRva)
        {
            std::cout << "[WARN] AntiCensor: unsupported CN asset index reader.\n";
            return false;
        }

        auto* comparison = base + CompareRva;
        AntiCensorSelectBundle = base + SelectBundleRva;
        if (MH_CreateHook(comparison, reinterpret_cast<void*>(&AntiCensorCompareRegion),
                          &AntiCensorOriginalComparison) != MH_OK)
        {
            std::cout << "[WARN] AntiCensor: failed to create the asset region hook.\n";
            return false;
        }
        if (MH_EnableHook(comparison) != MH_OK)
        {
            MH_RemoveHook(comparison);
            std::cout << "[WARN] AntiCensor: failed to enable the asset region hook.\n";
            return false;
        }
        std::cout << "[SCAN] AntiCensor asset region hook enabled.\n";
        return true;
    }

    void InitStoryCostumes(bool isOS)
    {
        using namespace Patterns::AntiCensor;
        using namespace Offsets::AntiCensor;

        void* lookup = nullptr;
        if (Compatibility::GetImageSize() == (isOS ? ImageSizeOS : ImageSize))
        {
            lookup = Compatibility::ResolveFunctionEntry(
                isOS ? GetResourceIdentifierRvaOS : GetResourceIdentifierRva,
                GetResourceIdentifier, GetResourceIdentifierCheckSize);
        }
        if (!lookup ||
            Scanner::ScanUniqueMainMod(PackResourceIdentifier,
                static_cast<uint8_t*>(lookup) + PackResourceIdentifierOffset,
                PackResourceIdentifierCheckSize) != static_cast<uint8_t*>(lookup) + PackResourceIdentifierOffset)
        {
            std::cout << "[WARN] AntiCensor: unsupported story costume resource lookup.\n";
            return;
        }
        if (MH_CreateHook(lookup, reinterpret_cast<void*>(&ResolveStoryCostume),
                          reinterpret_cast<void**>(&originalResourceIdentifier)) != MH_OK)
        {
            std::cout << "[WARN] AntiCensor: failed to create the story costume hook.\n";
            return;
        }
        if (MH_EnableHook(lookup) != MH_OK)
        {
            MH_RemoveHook(lookup);
            std::cout << "[WARN] AntiCensor: failed to enable the story costume hook.\n";
            return;
        }
        std::cout << "[SCAN] AntiCensor story costume hook enabled.\n";
    }
}

void AntiCensor::Init()
{
    const auto game = GetModuleHandleW(nullptr);
    if (GetModuleHandleW(L"YuanShen.exe") == game)
    {
        if (InitRegionalAssets(reinterpret_cast<uint8_t*>(game)))
            InitStoryCostumes(false);
    }
    else if (GetModuleHandleW(L"GenshinImpact.exe") == game)
    {
        InitStoryCostumes(true);
    }
}
