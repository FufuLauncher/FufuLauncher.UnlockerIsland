/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once
#include <functional>
#include <iostream>
#include "../il2cpp/Il2CppObject.h"

struct Il2CppString
{
    Il2CppObject object;
    int32_t length;
    wchar_t chars[1];
};

namespace Detail
{
    inline void Runner(void* funcPtr)
    {
        auto* f = static_cast<std::function<void()>*>(funcPtr);
        (*f)();
    }

    inline void SecureExecutor(void* funcPtr, void (*runner)(void*))
    {
        __try
        {
            runner(funcPtr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            OutputDebugStringA("[Utils] SafeInvoke caught a crash!\n");
        }
    }
}

inline void SafeInvoke(std::function<void()> func)
{
    Detail::SecureExecutor(&func, Detail::Runner);
}

template <typename T>
bool IsValid(T ptr)
{
    if (!ptr) return false;
    return (uintptr_t)ptr > 0x10000 && (uintptr_t)ptr < 0x7FFFFFFFFFFF;
}

template <typename T>
bool IsValidCodePtr(T ptr)
{
    if (!ptr) return false;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((LPCVOID)ptr, &mbi, sizeof(mbi)))
    {
        if (mbi.State == MEM_COMMIT && (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
            PAGE_EXECUTE_WRITECOPY)))
        {
            return true;
        }
    }
    return false;
}
