#include <Windows.h>
#include <thread>
#include <iostream>
#include <cstdio>
#include <psapi.h>
#include <wininet.h>
#include <string>
#include "Config.h"
#include "Hooks.h"
#include "SecurityUtils.h"

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "wininet.lib") 

const char* AUTH_URL = "https://philia093.cyou/Unlock.json";

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* pExceptionInfo) {
    std::cout << "\n\n[!] CRASH DETECTED" << std::endl;
    std::cout << "Exception Code: 0x" << std::hex << pExceptionInfo->ExceptionRecord->ExceptionCode << std::endl;
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
)" << std::endl;
        std::cout << "有一定几率在加载页面卡死，也有一定几率在退出个人主页时崩溃，重启即可解决" << std::endl;
        std::cout << "本项目开源地址: https://github.com/CodeCubist/FufuLauncher.UnlockerIsland" << std::endl; 
        std::cout << "爱来自FufuLauncher" << std::endl;
        std::cout << "[+] Console Allocated." << std::endl;
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
    char currentProcPath[MAX_PATH] = { 0 };
    std::string sPath;
    std::string sName;

    hMapFile = OpenFileMappingW(FILE_MAP_READ, FALSE, SHARED_MEM_NAME);
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
    Config::Load();

    if (Config::Get().debug_console) {
        OpenConsole("Unlocker Heartbeat System");
    }
    
    std::cout << Config::Get().hide_quest_banner << std::endl;
    
    std::cout << "[*] Initializing local security..." << std::endl;
    PerformSecurityCheck();
    
    std::thread([]() {
        while (true) {
            AuthResult res = CheckRemoteStatus();

            if (res == AuthResult::FAILED) {
                
                if (Config::Get().debug_console)
                    std::cout << "[!] Access Revoked! Terminating..." << std::endl;
                
                TerminateProcess(GetCurrentProcess(), 0);
                _exit(0);
            } 
            else if (res == AuthResult::NET_ERROR) {
                if (Config::Get().debug_console)
                    std::cout << "[!] Server unreachable." << std::endl;
                
                Sleep(5 * 60 * 1000); 
            } 
            else {
                if (Config::Get().debug_console)
                    std::cout << "[+] Heartbeat OK." << std::endl;
                
                Sleep(60 * 1000);
            }
        }
    }).detach();

    std::cout << "[*] Initializing Hooks..." << std::endl;
    if (!Hooks::Init()) {
        std::cout << "[!] Hooks::Init Failed!" << std::endl;
        return;
    }
    
    Hooks::InitHSRFps();
    
    std::cout << "[*] Waiting for GameUpdate..." << std::endl;
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
                std::cout << "[Network] >>> STATUS: DISCONNECTED (Blocking)" << std::endl;
            } else {
                Beep(1000, 200); 
                std::cout << "[Network] >>> STATUS: CONNECTED (Normal)" << std::endl;
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