/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include <windows.h>
#include <shlwapi.h>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>
#include "Launcher.h"

#pragma comment(lib, "shlwapi.lib")

const wchar_t* PLUGINS_SUBDIR_NAME = L"Plugins";

std::string WStringToString(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring GetCurrentDllDirectory()
{
    HMODULE hModule = NULL;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)&GetCurrentDllDirectory,
        &hModule);

    if (hModule != NULL)
    {
        wchar_t modulePath[MAX_PATH];
        if (GetModuleFileNameW(hModule, modulePath, MAX_PATH) > 0)
        {
            std::wstring path(modulePath);
            size_t lastSlash = path.find_last_of(L"\\/");
            if (lastSlash != std::wstring::npos)
            {
                return path.substr(0, lastSlash);
            }
        }
    }
    return L".";
}

std::wstring GetLogFilePath()
{
    std::wstring dllDir = GetCurrentDllDirectory();
    return dllDir + L"\\Launcher.log";
}

void WriteLog(const std::string& message)
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time_t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    std::ofstream logFile(GetLogFilePath(), std::ios::app);
    if (logFile.is_open())
    {
        logFile << "[" << ss.str() << "] " << message << std::endl;
    }
}

void HideConsole()
{
    HWND hwnd = GetConsoleWindow();
    if (hwnd != NULL) ShowWindow(hwnd, SW_HIDE);
}

bool InjectDll(HANDLE hProcess, const std::wstring& dllPath)
{
    if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) return false;

    std::wstring injectPath = dllPath;
    DWORD shortPathLen = GetShortPathNameW(dllPath.c_str(), nullptr, 0);
    if (shortPathLen > 0)
    {
        std::vector<wchar_t> shortPath(shortPathLen);
        if (GetShortPathNameW(dllPath.c_str(), shortPath.data(), shortPathLen) > 0)
        {
            injectPath = shortPath.data();
        }
    }

    size_t size = (injectPath.length() + 1) * sizeof(wchar_t);
    LPVOID remoteMem = VirtualAllocEx(hProcess, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) return false;

    if (!WriteProcessMemory(hProcess, remoteMem, injectPath.c_str(), size, nullptr))
    {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        return false;
    }

    LPTHREAD_START_ROUTINE loadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(
        GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, loadLibrary, remoteMem, 0, nullptr);
    if (!hThread)
    {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hThread);

    return exitCode != 0;
}

std::vector<std::wstring> ExtractDllsFromIni(const std::wstring& iniPath)
{
    std::vector<std::wstring> dllList;
    std::ifstream file(iniPath);
    std::string line;

    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos)
        {
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);

            auto trim = [](std::string& s)
            {
                size_t start = s.find_first_not_of(" \t\"'");
                if (start == std::string::npos)
                {
                    s.clear();
                }
                else
                {
                    size_t end = s.find_last_not_of(" \t\"'");
                    s = s.substr(start, end - start + 1);
                }
            };

            trim(key);
            if (_stricmp(key.c_str(), "File") == 0)
            {
                trim(value);
                if (value.length() >= 4)
                {
                    std::string ext = value.substr(value.length() - 4);
                    if (_stricmp(ext.c_str(), ".dll") == 0)
                    {
                        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &value[0], (int)value.size(), NULL, 0);
                        std::wstring wstrTo(size_needed, 0);
                        MultiByteToWideChar(CP_UTF8, 0, &value[0], (int)value.size(), &wstrTo[0], size_needed);

                        bool exists = false;
                        for (const auto& existingDll : dllList)
                        {
                            if (_wcsicmp(existingDll.c_str(), wstrTo.c_str()) == 0)
                            {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists)
                        {
                            dllList.push_back(wstrTo);
                        }
                    }
                }
            }
        }
    }
    return dllList;
}

void RecursiveScanAndInject(HANDLE hProcess, const std::wstring& directory, int& injectedCount)
{
    std::wstring searchPath = directory + L"\\*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do
    {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            continue;

        std::wstring fullPath = directory + L"\\" + findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            RecursiveScanAndInject(hProcess, fullPath, injectedCount);
        }
        else
        {
            if (_wcsicmp(findData.cFileName, L"config.ini") == 0)
            {
                std::vector<std::wstring> targetDllNames = ExtractDllsFromIni(fullPath);

                for (const std::wstring& targetDllName : targetDllNames)
                {
                    if (!targetDllName.empty())
                    {
                        std::wstring targetDllPath = directory + L"\\" + targetDllName;

                        if (GetFileAttributesW(targetDllPath.c_str()) != INVALID_FILE_ATTRIBUTES)
                        {
                            std::string sFileName = WStringToString(targetDllName);
                            WriteLog(
                                "[INFO] Plugin referenced by configuration file discovered: " + sFileName +
                                ". Initiating injection...");

                            if (InjectDll(hProcess, targetDllPath))
                            {
                                WriteLog("[INFO] Plugin injection completed successfully: " + sFileName);
                                injectedCount++;
                            }
                            else
                            {
                                WriteLog("[ERROR] Plugin injection failed: " + sFileName);
                            }
                        }
                        else
                        {
                            std::string sFileName = WStringToString(targetDllName);
                            WriteLog("[WARN] Plugin file referenced by configuration file not found: " + sFileName);
                        }
                    }
                }
            }
        }
    }
    while (FindNextFileW(hFind, &findData) != 0);

    FindClose(hFind);
}

void InjectPlugins(HANDLE hProcess)
{
    std::wstring dllDir = GetCurrentDllDirectory();
    std::wstring pluginsDir = dllDir + L"\\" + PLUGINS_SUBDIR_NAME;

    if (GetFileAttributesW(pluginsDir.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        CreateDirectoryW(pluginsDir.c_str(), NULL);
        WriteLog("[INFO] Plugins directory created: " + WStringToString(pluginsDir));
    }

    std::wstring offsetJsonPath = pluginsDir + L"\\offset.json";
    if (GetFileAttributesW(offsetJsonPath.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        WriteLog("[INFO] Stale artifact from a previous download detected: offset.json. Removing...");
        if (DeleteFileW(offsetJsonPath.c_str()))
        {
            WriteLog("[INFO] Stale artifact offset.json removed successfully.");
        }
        else
        {
            WriteLog(
                "[WARN] Failed to remove stale artifact offset.json. Error code: " + std::to_string(GetLastError()));
        }
    }

    WriteLog("[INFO] Recursively scanning plugin directory for config.ini entries: " + WStringToString(pluginsDir));

    int totalInjected = 0;
    RecursiveScanAndInject(hProcess, pluginsDir, totalInjected);

    WriteLog("[INFO] Plugin loading phase finished. Total plugins injected: " + std::to_string(totalInjected));
}

extern "C" {
LAUNCHER_API int LaunchGameAndInject(const wchar_t* gamePath, const wchar_t* dllPath, const wchar_t* commandLineArgs,
                                     wchar_t* errorMessage, int errorMessageSize)
{
    WriteLog("Launcher session started");
    HideConsole();

    if (!ValidateGamePath(gamePath))
    {
        if (errorMessage) wcsncpy_s(errorMessage, errorMessageSize, L"Invalid game executable path.", _TRUNCATE);
        WriteLog("[ERROR] Invalid game executable path.");
        return 1;
    }

    std::wstring wGamePath = gamePath;
    std::wstring workingDir = wGamePath.substr(0, wGamePath.find_last_of(L"\\/"));

    std::wstring cmdArgs = commandLineArgs ? commandLineArgs : L"";
    wchar_t* pCmdLine = nullptr;
    if (!cmdArgs.empty())
    {
        pCmdLine = new wchar_t[cmdArgs.size() + 1];
        wcscpy_s(pCmdLine, cmdArgs.size() + 1, cmdArgs.c_str());
    }

    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(gamePath, pCmdLine, nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, workingDir.c_str(), &si,
                        &pi))
    {
        if (errorMessage) wcsncpy_s(errorMessage, errorMessageSize, L"Failed to create the game process.", _TRUNCATE);
        WriteLog("[ERROR] CreateProcessW invocation failed. Error code: " + std::to_string(GetLastError()));
        if (pCmdLine) delete[] pCmdLine;
        return 3;
    }

    InjectPlugins(pi.hProcess);

    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    if (pCmdLine) delete[] pCmdLine;

    WriteLog("Launcher session completed");
    return 0;
}

LAUNCHER_API int GetDefaultDllPath(wchar_t* dllPath, int dllPathSize)
{
    if (dllPath && dllPathSize > 0)
    {
        dllPath[0] = L'\0';
    }
    return 0;
}

LAUNCHER_API bool ValidateGamePath(const wchar_t* gamePath)
{
    return gamePath && PathFileExistsW(gamePath);
}

LAUNCHER_API bool ValidateDllPath(const wchar_t* dllPath)
{
    return dllPath && PathFileExistsW(dllPath);
}
}
