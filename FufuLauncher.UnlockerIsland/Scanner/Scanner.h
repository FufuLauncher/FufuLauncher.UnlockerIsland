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

    // Unique match in executable sections of the main module; optionally
    // restrict the search to a function/range. Missing or ambiguous => nullptr.
    void* ScanUniqueMainMod(const std::string& signature, void* within = nullptr, size_t size = 0);

    void* ResolveRelative(void* instruction, int offset = 1, int instrSize = 5);
}
