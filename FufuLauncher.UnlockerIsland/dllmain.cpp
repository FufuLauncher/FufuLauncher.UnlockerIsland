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

#include "Config.h"
#include "Hooks.h"

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "wininet.lib") 
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")
#ifndef CALG_SHA256
#define CALG_SHA256 0x0000800C
#endif

const char* AUTH_URL = "https://philia093.cyou/Unlock.json";

namespace LicenseSystem {
    
    std::string GetHWID() {
        DWORD serialNum = 0;
        GetVolumeInformationA("C:\\", NULL, 0, &serialNum, NULL, NULL, NULL, 0);
        std::stringstream ss;
        ss << std::hex << std::uppercase << serialNum;
        return ss.str();
    }
    
    std::string CalculateSHA256(const std::string& data) {
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        BYTE rgbHash[32];
        DWORD cbHash = 32;
        std::string hashStr = "";

        if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
            if (CryptCreateHash(hProv, CALG_SHA256, 0, 0, &hHash)) {
                if (CryptHashData(hHash, (BYTE*)data.c_str(), (DWORD)data.length(), 0)) {
                    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
                        std::stringstream ss;
                        for (DWORD i = 0; i < cbHash; i++) {
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
    
    std::string Base64Encode(const std::vector<BYTE>& data) {
        DWORD dwLen = 0;
        if (!CryptBinaryToStringA(data.data(), (DWORD)data.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &dwLen)) return "";
        
        std::string buffer(dwLen, '\0');
        if (!CryptBinaryToStringA(data.data(), (DWORD)data.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &buffer[0], &dwLen)) return "";
        
        return buffer;
    }
    
    std::vector<BYTE> CaptureScreen() {
        int w = GetSystemMetrics(SM_CXSCREEN);
        int h = GetSystemMetrics(SM_CYSCREEN);
        HDC hScreen = GetDC(NULL);
        HDC hDC = CreateCompatibleDC(hScreen);
        HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
        SelectObject(hDC, hBitmap);
        BitBlt(hDC, 0, 0, w, h, hScreen, 0, 0, SRCCOPY);

        BITMAP bmpScreen;
        GetObject(hBitmap, sizeof(BITMAP), &bmpScreen);
        
        BITMAPFILEHEADER   bmfHeader;
        BITMAPINFOHEADER   bi;
        
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

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* pExceptionInfo) {
    std::cout << "\n\n[!] CRASH DETECTED" << '\n';
    std::cout << "Exception Code: 0x" << std::hex << pExceptionInfo->ExceptionRecord->ExceptionCode << '\n';
    return EXCEPTION_CONTINUE_SEARCH;
}

void OpenConsole(const char* title) {
    if (AllocConsole()) {
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
        std::cout << "有一定几率在加载页面卡死，也有一定几率在退出个人主页时崩溃，重启即可解决" << '\n';
        std::cout << "本项目开源地址: https://github.com/CodeCubist/FufuLauncher.UnlockerIsland" << '\n'; 
        std::cout << "爱来自FufuLauncher" << '\n';
        std::cout << "[+] Console Allocated." << '\n';
    }
}

enum class AuthResult {
    SUCCESS,    
    FAILED,     
    NET_ERROR   
};

AuthResult CheckRemoteStatus() {
    AuthResult result = AuthResult::NET_ERROR;
    HINTERNET hInternet = InternetOpenA("FufuLauncher Unlock/1.0.1", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    
    if (hInternet) {
        DWORD timeout = 5000;
        InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(DWORD));

        HINTERNET hConnect = InternetOpenUrlA(hInternet, AUTH_URL, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
        if (hConnect) {
            char buffer[512];
            DWORD bytesRead;
            if (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string response = buffer;

                if (response.find("\"Status\": \"true\"") != std::string::npos) {
                    result = AuthResult::SUCCESS;
                } else if (response.find("\"Status\": \"false\"") != std::string::npos) {
                    result = AuthResult::FAILED;
                }
            }
            InternetCloseHandle(hConnect);
        }
        InternetCloseHandle(hInternet);
    }
    return result;
}

void MainWorker(HMODULE hMod) {

    Config::Load();

    if (Config::Get().debug_console) {
        OpenConsole("Unlocker Heartbeat System");
    }
    
    std::cout << Config::Get().hide_quest_banner << '\n';
    
    std::thread([]
    {
        while (true) {
            AuthResult res = CheckRemoteStatus();

            if (res == AuthResult::FAILED) {
                
                if (Config::Get().debug_console)
                    std::cout << "[!] Access Revoked! Terminating..." << '\n';
                
                TerminateProcess(GetCurrentProcess(), 0);
                _exit(0);
            } 
            else if (res == AuthResult::NET_ERROR) {
                if (Config::Get().debug_console)
                    std::cout << "[!] Server unreachable." << '\n';
                
                Sleep(5 * 60 * 1000); 
            } 
            else {
                if (Config::Get().debug_console)
                    std::cout << "[+] Heartbeat OK." << '\n';
                
                Sleep(60 * 1000);
            }
        }
    }).detach();

    std::cout << "[*] Initializing Hooks..." << '\n';
    if (!Hooks::Init()) {
        std::cout << "[!] Hooks::Init Failed!" << '\n';
        return;
    }
    
    std::cout << "[*] Waiting for GameUpdate..." << '\n';
    while (!Hooks::IsGameUpdateInit()) {
        Sleep(1000);
    }

    while (true) {
        auto& cfg = Config::Get();
        
        static bool net_was_pressed = false;
        bool net_is_pressed = (GetAsyncKeyState(cfg.network_toggle_key) & 0x8000);

        if (net_is_pressed && !net_was_pressed) {
            cfg.is_currently_blocking = !cfg.is_currently_blocking;
            
            if (cfg.is_currently_blocking) {
                Beep(300, 500); 
                std::cout << "[Network] >>> STATUS: DISCONNECTED (Blocking)" << '\n';
            } else {
                Beep(1000, 200); 
                std::cout << "[Network] >>> STATUS: CONNECTED (Normal)" << '\n';
            }
        }
        net_was_pressed = net_is_pressed;
        
        if (GetAsyncKeyState(cfg.toggle_key) & 0x8000) {
            Config::Load();
            Hooks::TriggerReloadPopup();
            Sleep(500);
        }
        
        if (GetAsyncKeyState(cfg.toggle_key) & 0x8000) {
            Config::Load();
            Sleep(500);
        }
        if (cfg.craft_key != 0 && (GetAsyncKeyState(cfg.craft_key) & 0x8000)) {
            Hooks::RequestOpenCraft();
            Sleep(500);
        }
        
        Sleep(100);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
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
* */