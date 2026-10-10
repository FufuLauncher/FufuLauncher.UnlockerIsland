/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once

#ifdef LAUNCHER_EXPORTS
#define LAUNCHER_API __declspec(dllexport)
#else
#define LAUNCHER_API __declspec(dllimport)
#endif

extern "C" {
LAUNCHER_API int LaunchGameAndInject(const wchar_t* gamePath, const wchar_t* dllPath, const wchar_t* commandLineArgs,
                                     wchar_t* errorMessage, int errorMessageSize);

LAUNCHER_API int GetDefaultDllPath(wchar_t* dllPath, int dllPathSize);
LAUNCHER_API bool ValidateGamePath(const wchar_t* gamePath);
LAUNCHER_API bool ValidateDllPath(const wchar_t* dllPath);
}
