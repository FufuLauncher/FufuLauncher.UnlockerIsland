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
#include "SecurityUtils.h"

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
    
    void CheckAndVerify() {
        char currentPath[MAX_PATH];
        GetModuleFileNameA(NULL, currentPath, MAX_PATH);
        std::string pathStr = currentPath;
        std::string dir = pathStr.substr(0, pathStr.find_last_of("\\/"));
        
        bool found = false;
        namespace fs = std::filesystem;

        try {
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (filename.length() == 64) {
                        std::ifstream f(entry.path(), std::ios::binary);
                        if (!f.is_open()) continue;

                        std::stringstream buffer;
                        buffer << f.rdbuf();
                        std::string content = buffer.str();
                        f.close();
                        
                        std::string calculatedHash = CalculateSHA256(content);

                        if (calculatedHash == filename) {
                            found = true;
                            break;
                        }
                        try {
                            fs::remove(entry.path());
                        } catch (...) {
                            
                        }
                    }
                }
            }
        } catch (...) {
            
        }

        if (found) {
            return;
        }
        
        const char* disclaimer = 
            "【风险警告与免责声明】\n\n"
            "! ! !使用非官方修改工具严重违反游戏的服务条款，并可能导致您的账号被永久封禁! ! !\n\n"
            "为了验证您的身份并防止滥用，我们将记录以下信息到本地认证文件\n"
            "文件均保存在本地，我们不会上传你的任何信息到服务器\n"
            "1. 您的硬件ID (证明你的电脑)\n"
            "2. 当前屏幕截图 (防纠纷)\n\n"
            "点击【是】即表示您已知晓所有风险，自愿承担账号封禁、数据丢失等一切后果"
            "并同意我们生成本地认证文件\n\n"
            "点击【否】将立即退出程序";

        int result = MessageBoxA(NULL, disclaimer, "FufuLauncher Unlocker", MB_YESNO | MB_ICONWARNING | MB_TOPMOST);

        if (result != IDYES) {
            TerminateProcess(GetCurrentProcess(), 0);
            return;
        }
        
        std::string hwid = GetHWID();
        std::vector<BYTE> screenRaw = CaptureScreen();
        std::string screenBase64 = Base64Encode(screenRaw);

        std::string fileContent = hwid + "|" + screenBase64;
        std::string fileHash = CalculateSHA256(fileContent);

        std::string targetPath = dir + "\\" + fileHash;
        std::ofstream outfile(targetPath, std::ios::binary);
        if (outfile.is_open()) {
            outfile << fileContent;
            outfile.close();
        } else {
            MessageBoxA(NULL, "无法写入认证文件", "错误", MB_OK | MB_ICONERROR);
            TerminateProcess(GetCurrentProcess(), 0);
        }
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

void PerformSecurityCheck() {
    bool isVerified = false;
    std::string failReason = "未知错误";

    HANDLE hMapFile;
    void* pBuf = NULL;
    AuthPacket pkt = {};
    char currentProcPath[MAX_PATH] = {};
    std::string sPath;
    std::string sName;

    hMapFile = OpenFileMappingW(FILE_MAP_READ, FALSE, shared_mem_name);
    if (hMapFile == NULL) {
        failReason = "无法连接通道 (Code: " + std::to_string(GetLastError()) + ")";
        goto FAILED;
    }

    pBuf = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(AuthPacket));
    if (pBuf == NULL) {
        failReason = "无法读取验证数据";
        CloseHandle(hMapFile);
        goto FAILED;
    }

    CopyMemory(&pkt, pBuf, sizeof(AuthPacket));
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);

    if (pkt.magic_header != 0xDEADBEEFCAFEBABE) {
        failReason = "非法的数据头";
        goto FAILED;
    }

    SecurityCrypto::ProcessBuffer((uint8_t*)&pkt.target_pid, ENCRYPTED_SIZE, pkt.salt);

    if (pkt.target_pid != GetCurrentProcessId()) {
        failReason = "数据不匹配";
        goto FAILED;
    }

    if (pkt.checksum != SecurityCrypto::CalcChecksum(&pkt)) {
        failReason = "完整性校验失败";
        goto FAILED;
    }

    GetModuleFileNameA(NULL, currentProcPath, MAX_PATH);
    sPath = currentProcPath;
    sName = sPath.substr(sPath.find_last_of("\\/") + 1);

    if (strcmp(pkt.process_name, sName.c_str()) != 0) {
        failReason = "非法宿主进程: " + sName;
        goto FAILED;
    }

    isVerified = true;

FAILED:
    if (!isVerified) {
    }
}

void MainWorker(HMODULE hMod) {
    LicenseSystem::CheckAndVerify();

    Config::Load();

    if (Config::Get().debug_console) {
        OpenConsole("Unlocker Heartbeat System");
    }
    
    std::cout << Config::Get().hide_quest_banner << '\n';
    
    std::cout << "[*] Initializing local security..." << '\n';
    PerformSecurityCheck();
    
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
    
    Hooks::InitHSRFps();
    
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
        
        Hooks::UpdateHSRFps();
        
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
*   _____            __           _                                      _                         _   _           _                  _                    ___         _                       _ 
*  |  ___|  _   _   / _|  _   _  | |       __ _   _   _   _ __     ___  | |__     ___   _ __      | | | |  _ __   | |   ___     ___  | | __   ___   _ __  |_ _|  ___  | |   __ _   _ __     __| |
*  | |_    | | | | | |_  | | | | | |      / _` | | | | | | '_ \   / __| | '_ \   / _ \ | '__|     | | | | | '_ \  | |  / _ \   / __| | |/ /  / _ \ | '__|  | |  / __| | |  / _` | | '_ \   / _` |
*  |  _|   | |_| | |  _| | |_| | | |___  | (_| | | |_| | | | | | | (__  | | | | |  __/ | |     _  | |_| | | | | | | | | (_) | | (__  |   <  |  __/ | |     | |  \__ \ | | | (_| | | | | | | (_| |
*  |_|      \__,_| |_|    \__,_| |_____|  \__,_|  \__,_| |_| |_|  \___| |_| |_|  \___| |_|    (_)  \___/  |_| |_| |_|  \___/   \___| |_|\_\  \___| |_|    |___| |___/ |_|  \__,_| |_| |_|  \__,_|
*                                                                                                                                                                                                
*/