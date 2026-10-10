/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include <Windows.h>
#include <thread>
#include <iostream>
#include <cstdio>
#include <psapi.h>
#include <wininet.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <wincrypt.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <mutex>
#include <atomic>
#include <algorithm>

#include "../Config/Config.h"
#include "Hooks.h"

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")
#ifndef CALG_SHA256
#define CALG_SHA256 0x0000800C
#endif

static std::string g_LogPath;

static void LogToFile(const std::string& msg)
{
    std::error_code ec;
    if (std::filesystem::exists(g_LogPath, ec))
    {
        if (std::filesystem::file_size(g_LogPath, ec) > 1048576)
        {
            std::ofstream truncFile(g_LogPath, std::ios::trunc);
            truncFile.close();
        }
    }

    std::ofstream ofs(g_LogPath, std::ios::app);
    if (ofs.is_open())
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        char timeBuf[64];
        sprintf_s(timeBuf, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
        ofs << timeBuf << msg << std::endl;
    }
}

static bool IsSystemModulePath(const std::string& path)
{
    char windowsDir[MAX_PATH];
    UINT length = GetSystemWindowsDirectoryA(windowsDir, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return false;

    std::string prefix(windowsDir, length);
    prefix += '\\';

    return path.size() >= prefix.size() && _strnicmp(path.c_str(), prefix.c_str(), prefix.size()) == 0;
}

static void LogInjectedModules()
{
    HMODULE modules[1024];
    DWORD needed = 0;
    if (!EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &needed))
    {
        LogToFile("Injected module enumeration failed, error " + std::to_string(GetLastError()));
        return;
    }

    size_t count = needed / sizeof(HMODULE);
    constexpr size_t kModuleCapacity = sizeof(modules) / sizeof(modules[0]);
    if (count > kModuleCapacity) count = kModuleCapacity;

    constexpr size_t kMaxLogged = 32;
    HMODULE mainModule = GetModuleHandleA(nullptr);
    std::vector<std::string> injected;

    for (size_t i = 0; i < count; ++i)
    {
        if (modules[i] == mainModule) continue;

        char path[MAX_PATH];
        if (GetModuleFileNameExA(GetCurrentProcess(), modules[i], path, MAX_PATH) == 0) continue;
        if (IsSystemModulePath(path)) continue;

        injected.emplace_back(path);
    }

    std::ostringstream log;
    log << "Injected DLLs detected (non-system modules): " << injected.size();
    for (size_t i = 0; i < injected.size() && i < kMaxLogged; ++i)
    {
        log << "\n  " << injected[i];
    }
    if (injected.size() > kMaxLogged)
    {
        log << "\n  ... and " << (injected.size() - kMaxLogged) << " more";
    }
    LogToFile(log.str());
}

std::atomic<bool> g_ShouldShowDialog{false};
std::atomic<bool> g_StopDialogPolling{false};
std::string g_DialogText = "";
std::mutex g_DialogMutex;

void DialogWorker()
{
    std::string lastText = "";
    while (true)
    {
        if (g_StopDialogPolling.load())
        {
            break;
        }

        HINTERNET hInternet = InternetOpenA("FufuLauncher Unlock/1.7.0.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (hInternet)
        {
            DWORD timeout = 5000;
            InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(DWORD));

            HINTERNET hConnect = InternetOpenUrlA(hInternet, "https://fu1.fun/dialog.json", NULL, 0,
                                                  INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
            if (hConnect)
            {
                char buffer[1024];
                DWORD bytesRead;
                std::string response = "";

                while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    response += buffer;
                }

                if (!response.empty())
                {
                    std::regex dialogRegex(R"REGEX("dialog"\s*:\s*(true|false))REGEX");
                    std::regex textRegex(R"REGEX("text"\s*:\s*"((?:\\.|[^"\\])*)")REGEX");
                    std::smatch match;
                    bool isDialog = false;

                    if (std::regex_search(response, match, dialogRegex))
                    {
                        if (match[1].str() == "true")
                        {
                            isDialog = true;
                        }
                    }

                    if (isDialog && std::regex_search(response, match, textRegex))
                    {
                        std::string currentText = match[1].str();

                        size_t pos = 0;
                        while ((pos = currentText.find("\\n", pos)) != std::string::npos)
                        {
                            currentText.replace(pos, 2, "\n");
                            pos += 1;
                        }
                        pos = 0;
                        while ((pos = currentText.find("\\\"", pos)) != std::string::npos)
                        {
                            currentText.replace(pos, 2, "\"");
                            pos += 1;
                        }

                        if (currentText != lastText)
                        {
                            lastText = currentText;
                            std::lock_guard<std::mutex> lock(g_DialogMutex);
                            g_DialogText = currentText;
                            g_ShouldShowDialog.store(true);
                        }
                    }
                }
                InternetCloseHandle(hConnect);
            }
            InternetCloseHandle(hInternet);
        }
        Sleep(5 * 60 * 1000);
    }
}

inline FILETIME GetFileLastWriteTime(const std::string& path)
{
    FILETIME lastWriteTime = {0, 0};
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fileInfo))
    {
        lastWriteTime = fileInfo.ftLastWriteTime;
    }
    return lastWriteTime;
}

const char* AUTH_URL = "https://fu1.fun/Unlock.json";

namespace LicenseSystem
{
    std::string GetHWID()
    {
        DWORD serialNum = 0;
        GetVolumeInformationA("C:\\", NULL, 0, &serialNum, NULL, NULL, NULL, 0);
        std::stringstream ss;
        ss << std::hex << std::uppercase << serialNum;
        return ss.str();
    }

    std::string CalculateSHA256(const std::string& data)
    {
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        BYTE rgbHash[32];
        DWORD cbHash = 32;
        std::string hashStr = "";

        if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        {
            if (CryptCreateHash(hProv, CALG_SHA256, 0, 0, &hHash))
            {
                if (CryptHashData(hHash, (BYTE*)data.c_str(), (DWORD)data.length(), 0))
                {
                    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
                    {
                        std::stringstream ss;
                        for (DWORD i = 0; i < cbHash; i++)
                        {
                            ss << std::hex << std::setw(2) << std::setfill('0') << (int)rgbHash[i];
                        }
                        hashStr = ss.str();
                    }
                }
                CryptDestroyHash(hHash);
            }
            CryptReleaseContext(hProv, 0);
        }
        return hashStr;
    }

    std::string Base64Encode(const std::vector<BYTE>& data)
    {
        DWORD dwLen = 0;
        if (!CryptBinaryToStringA(data.data(), (DWORD)data.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL,
                                  &dwLen)) return "";

        std::string buffer(dwLen, '\0');
        if (!CryptBinaryToStringA(data.data(), (DWORD)data.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                                  &buffer[0], &dwLen)) return "";

        return buffer;
    }

    std::vector<BYTE> CaptureScreen()
    {
        int w = GetSystemMetrics(SM_CXSCREEN);
        int h = GetSystemMetrics(SM_CYSCREEN);
        HDC hScreen = GetDC(NULL);
        HDC hDC = CreateCompatibleDC(hScreen);
        HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
        SelectObject(hDC, hBitmap);
        BitBlt(hDC, 0, 0, w, h, hScreen, 0, 0, SRCCOPY);

        BITMAP bmpScreen;
        GetObject(hBitmap, sizeof(BITMAP), &bmpScreen);

        BITMAPFILEHEADER bmfHeader;
        BITMAPINFOHEADER bi;

        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = bmpScreen.bmWidth;
        bi.biHeight = bmpScreen.bmHeight;
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;
        bi.biClrImportant = 0;

        DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
        std::vector<BYTE> lpbitmap(dwBmpSize);

        GetDIBits(hScreen, hBitmap, 0, (UINT)bmpScreen.bmHeight, lpbitmap.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
        bmfHeader.bfSize = dwSizeofDIB;
        bmfHeader.bfType = 0x4D42;

        std::vector<BYTE> finalData;
        finalData.reserve(dwSizeofDIB);

        BYTE* pHead = (BYTE*)&bmfHeader;
        finalData.insert(finalData.end(), pHead, pHead + sizeof(bmfHeader));

        BYTE* pInfo = (BYTE*)&bi;
        finalData.insert(finalData.end(), pInfo, pInfo + sizeof(bi));

        finalData.insert(finalData.end(), lpbitmap.begin(), lpbitmap.end());

        DeleteObject(hBitmap);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);

        return finalData;
    }
}

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* pExceptionInfo)
{
    std::cout << "\n\n[!] CRASH DETECTED" << '\n';
    std::cout << "Exception Code: 0x" << std::hex << pExceptionInfo->ExceptionRecord->ExceptionCode << '\n';
    return EXCEPTION_CONTINUE_SEARCH;
}

void OpenConsole(const char* title)
{
    if (AllocConsole())
    {
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);
        freopen_s(&f, "CONIN$", "r", stdin);
        SetConsoleTitleA(title);
        SetUnhandledExceptionFilter(CrashHandler);
        std::cout << R"(
 __        __  _____   _        ____    ___    __  __   _____ 
 \ \      / / | ____| | |      / ___|  / _ \  |  \/  | | ____|
  \ \ /\ / /  |  _|   | |     | |     | | | | | |\/| | |  _|  
   \ V  V /   | |___  | |___  | |___  | |_| | | |  | | | |___ 
    \_/\_/    |_____| |_____|  \____|  \___/  |_|  |_| |_____|
)" << '\n';
        std::cout << "GitHub: https://github.com/CodeCubist/FufuLauncher.UnlockerIsland" << '\n';
        std::cout << "FufuLauncher Project. Built with Love" << '\n';
        std::cout << "[+] Console Allocated" << '\n';
    }
}

enum class AuthResult
{
    SUCCESS,
    FAILED,
    NET_ERROR,
    BANNED_UID
};

std::vector<uint32_t> ReadUidsFromFile()
{
    std::vector<uint32_t> uids;

    char path[MAX_PATH];
    HMODULE hm = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)&g_LogPath, &hm);
    GetModuleFileNameA(hm, path, sizeof(path));

    std::string fullPath = path;
    std::string dirPath;
    size_t lastSlash = fullPath.find_last_of("\\/");
    if (lastSlash != std::string::npos)
    {
        dirPath = fullPath.substr(0, lastSlash);
    }
    else
    {
        dirPath = ".";
    }
    std::string jsonPath = dirPath + "\\uids.json";

    if (!std::filesystem::exists(jsonPath))
    {
        return uids;
    }

    std::ifstream ifs(jsonPath);
    if (!ifs.is_open())
    {
        return uids;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    size_t pos = 0;
    while ((pos = content.find("\"uid\"", pos)) != std::string::npos)
    {
        pos += 5;
        size_t colon = content.find(':', pos);
        if (colon == std::string::npos) break;
        size_t quoteStart = content.find('"', colon + 1);
        if (quoteStart == std::string::npos) break;
        size_t quoteEnd = content.find('"', quoteStart + 1);
        if (quoteEnd == std::string::npos) break;

        std::string uidStr = content.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        if (!uidStr.empty())
        {
            uint32_t uid = (uint32_t)strtoul(uidStr.c_str(), nullptr, 10);
            if (uid > 10000000)
            {
                uids.push_back(uid);
            }
        }
        pos = quoteEnd + 1;
    }

    return uids;
}

AuthResult CheckRemoteStatus(const std::vector<uint32_t>& uidList)
{
    AuthResult result = AuthResult::NET_ERROR;
    HINTERNET hInternet = InternetOpenA("FufuLauncher Unlock/1.7.0.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    if (hInternet)
    {
        DWORD timeout = 5000;
        InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(DWORD));

        HINTERNET hConnect = InternetOpenUrlA(hInternet, AUTH_URL, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE,
                                              0);
        if (hConnect)
        {
            char buffer[1024];
            DWORD bytesRead;
            std::string response = "";

            while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                response += buffer;
            }

            if (!response.empty())
            {
                if (response.find("\"Status\":\"false\"") != std::string::npos || response.find("\"Status\": \"false\"")
                    != std::string::npos)
                {
                    result = AuthResult::FAILED;
                }
                else if (response.find("\"Status\":\"true\"") != std::string::npos || response.find(
                    "\"Status\": \"true\"") != std::string::npos)
                {
                    result = AuthResult::SUCCESS;

                    if (!uidList.empty())
                    {
                        size_t arrayStart = response.find("\"BannedUIDs\"");
                        if (arrayStart != std::string::npos)
                        {
                            size_t arrayEnd = response.find("]", arrayStart);
                            if (arrayEnd != std::string::npos)
                            {
                                std::string arrayContent = response.substr(arrayStart, arrayEnd - arrayStart);
                                for (uint32_t uid : uidList)
                                {
                                    std::string uidStr = std::to_string(uid);
                                    if (arrayContent.find(uidStr) != std::string::npos)
                                    {
                                        result = AuthResult::BANNED_UID;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            InternetCloseHandle(hConnect);
        }
        InternetCloseHandle(hInternet);
    }
    return result;
}

void MainWorker(HMODULE hMod)
{
    char dllPath[MAX_PATH];
    GetModuleFileNameA(hMod, dllPath, MAX_PATH);
    g_LogPath = dllPath;
    size_t lastSlash = g_LogPath.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        g_LogPath = g_LogPath.substr(0, lastSlash + 1);
    g_LogPath += "heartbeat.log";
    LogToFile("=== DLL Loaded ===");

    Config::Load();

    if (Config::Get().debug_console)
    {
        OpenConsole("Unlocker Heartbeat System");
    }

    std::cout << Config::Get().hide_quest_banner << '\n';

    std::thread([]
    {
        while (!Hooks::IsGameUpdateInit())
        {
            Sleep(1000);
        }

        LogInjectedModules();
        std::thread(DialogWorker).detach();

        char szExeName[MAX_PATH];
        GetModuleFileNameA(NULL, szExeName, MAX_PATH);
        std::string exePath(szExeName);
        std::transform(exePath.begin(), exePath.end(), exePath.begin(), ::tolower);

        // Read UIDs from uids.json
        std::vector<uint32_t> uidList = ReadUidsFromFile();

        if (uidList.empty())
        {
            LogToFile("uids.json not found or empty, skipping UID check");
        }
        else
        {
            std::string uidLog = "UIDs loaded from file: ";
            for (size_t i = 0; i < uidList.size(); i++)
            {
                if (i > 0) uidLog += ", ";
                uidLog += std::to_string(uidList[i]);
            }
            LogToFile(uidLog);

            LogToFile("Checking ban for " + std::to_string(uidList.size()) + " UID(s)");
            AuthResult res = CheckRemoteStatus(uidList);
            LogToFile("AuthResult = " + std::to_string((int)res) + " (0=OK,1=FAIL,2=NET_ERR,3=BANNED)");

            if (res == AuthResult::FAILED || res == AuthResult::BANNED_UID)
            {
                LogToFile("!!! TERMINATING - " + std::string(
                    res == AuthResult::BANNED_UID ? "UID BANNED" : "ACCESS REVOKED"));
                TerminateProcess(GetCurrentProcess(), 0);
                _exit(0);
            }
            if (res == AuthResult::NET_ERROR)
            {
                LogToFile("Server unreachable");
            }
            else
            {
                LogToFile("Heartbeat OK");
            }
        }
    }).detach();

    std::cout << "[*] Initializing Hooks..." << '\n';
    if (!Hooks::Init())
    {
        std::cout << "[!] Hooks::Init Failed!" << '\n';
        return;
    }

    std::cout << "[*] Waiting for GameUpdate..." << '\n';
    while (!Hooks::IsGameUpdateInit())
    {
        Sleep(1000);
    }

    std::string configPath = Config::GetConfigPath();
    FILETIME lastConfigWriteTime = GetFileLastWriteTime(configPath);

    while (true)
    {
        auto& cfg = Config::Get();

        HWND hForeground = GetForegroundWindow();
        DWORD foregroundProcessId = 0;
        if (hForeground)
        {
            GetWindowThreadProcessId(hForeground, &foregroundProcessId);
        }
        bool isFocused = (foregroundProcessId == GetCurrentProcessId());

        if (isFocused && (GetAsyncKeyState(cfg.toggle_key) & 0x8000))
        {
            Config::Load();
            Hooks::TriggerReloadPopup();
            Sleep(500);
        }

        if (isFocused && cfg.craft_key != 0 && (GetAsyncKeyState(cfg.craft_key) & 0x8000))
        {
            Hooks::RequestOpenCraft();
            Sleep(500);
        }
        FILETIME currentWriteTime = GetFileLastWriteTime(configPath);

        if (CompareFileTime(&lastConfigWriteTime, &currentWriteTime) != 0)
        {
            Sleep(100);

            Config::Load();
            Hooks::TriggerReloadPopup();

            if (Config::Get().debug_console)
            {
                std::cout << "[*] Automatic reload" << '\n';
            }

            lastConfigWriteTime = GetFileLastWriteTime(configPath);
        }

        Sleep(100);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        std::thread(MainWorker, hModule).detach();
    }
    return TRUE;
}

/***
* _____            __           _                                      _                         _   _           _                  _                    ___         _                       _ 
* |  ___|  _   _   / _|  _   _  | |       __ _   _   _   _ __     ___  | |__     ___   _ __      | | | |  _ __   | |   ___     ___  | | __   ___   _ __  |_ _|  ___  | |   __ _   _ __     __| |
* | |_    | | | | | |_  | | | | | |      / _` | | | | | | '_ \   / __| | '_ \   / _ \ | '__|     | | | | | '_ \  | |  / _ \   / __| | |/ /  / _ \ | '__|  | |  / __| | |  / _` | | '_ \   / _` |
* |  _|   | |_| | |  _| | |_| | | |___  | (_| | | |_| | | | | | | (__  | | | | |  __/ | |     _  | |_| | | | | | | | | (_) | | (__  |   <  |  __/ | |     | |  \__ \ | | | (_| | | | | | | (_| |
* |_|      \__,_| |_|    \__,_| |_____|  \__,_|  \__,_| |_| |_|  \___| |_| |_|  \___| |_|    (_)  \___/  |_| |_| |_|  \___/   \___| |_|\_\  \___| |_|    |___| |___/ |_|  \__,_| |_| |_|  \__,_|
*
* 感谢项目https://github.com/SnapHutaoRemasteringProject/Snap.Hutao.Remastered.UnlockerIsland和其开发者对本项目的帮助
 */
