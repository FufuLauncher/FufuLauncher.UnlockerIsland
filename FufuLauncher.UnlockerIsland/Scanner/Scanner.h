/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once
#include <cstddef>
#include <string>

namespace Scanner
{
    void* ScanMainMod(const std::string& signature);

    void* ScanRange(void* start, size_t size, const std::string& signature);

    void* ResolveRelative(void* instruction, int offset = 1, int instrSize = 5);
}
