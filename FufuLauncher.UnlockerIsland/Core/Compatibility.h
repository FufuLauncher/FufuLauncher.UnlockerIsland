#pragma once
#include <Windows.h>
#include <cstdint>
#include "../Scanner/Scanner.h"

namespace Compatibility
{
    inline size_t GetImageSize()
    {
        auto* base = reinterpret_cast<const uint8_t*>(GetModuleHandleW(nullptr));
        auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (!base || dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0) return 0;
        auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)
            return 0;
        return nt->OptionalHeader.SizeOfImage;
    }

    inline void* ResolveFunctionEntry(uintptr_t rva, const char* signature, size_t length)
    {
        auto* base = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
        const auto imageSize = GetImageSize();
        if (!imageSize || rva >= imageSize || length > imageSize - rva) return nullptr;
        auto* address = base + rva;
        DWORD64 imageBase = 0;
        auto* function = RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(address), &imageBase, nullptr);
        if (!function || imageBase != reinterpret_cast<DWORD64>(base) ||
            function->BeginAddress != rva ||
            Scanner::ScanUniqueMainMod(signature, address, length) != address)
            return nullptr;
        return address;
    }
}
