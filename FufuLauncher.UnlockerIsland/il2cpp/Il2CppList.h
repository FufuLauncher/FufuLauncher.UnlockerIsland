/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once

#include "Il2CppArray.h"
#include "Il2CppObject.h"

#pragma pack(push, 4)
template <typename T>
class Il2CppList
{
    Il2CppObject obj;
    Il2CppArray<T>* array;
    int size;
    int version;

public:
    inline T Get(int index)
    {
        return array->Get(index);
    }

    inline void Set(int index, T value)
    {
        array->Set(index, value);
    }

    inline void Remove(T value)
    {
        array->Remove(value);

        size--;
        version++;
    }

    inline int Count()
    {
        return size;
    }

    inline Il2CppArray<T>* Items()
    {
        return array;
    }

    inline void IncrementVersion()
    {
        ++version;
    }

    inline void RemoveAt(int index)
    {
        if (!array || index < 0 || index >= size) return;
        for (int i = index; i + 1 < size; ++i)
        {
            array->Set(i, array->Get(i + 1));
        }
        array->Set(size - 1, T{});
        --size;
        ++version;
    }
};
