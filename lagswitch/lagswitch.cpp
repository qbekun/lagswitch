#include <iostream>
#include <windows.h>
#include <windivert.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <iomanip>
#include <sstream>
#include <wininet.h>
#include <vector>
#include <comdef.h>
#include <Wbemidl.h>
#include <mmsystem.h>
#include <fstream>
#include <discord_rpc.h>
#include <ctime>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "WinDivert.lib")
#pragma comment(lib, "discord-rpc.lib")

// ==================== CONFIGURATION STRUCT ====================
struct Config {
    int activationKey = VK_CAPITAL;  // Default: Caps Lock
    int blockTime = 10;              // Default: 10 seconds
    bool discordRPC = true;          // Default: enabled
};

Config config;

// ==================== DISCORD RPC ====================
void InitializeDiscordRPC() {
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    Discord_Initialize("1385284129062523092", &handlers, 1, nullptr);
}

void UpdateDiscordRPC(int secondsLeft = 0, UINT64 blockedPackets = 0) {
    if (!config.discordRPC) return;

    DiscordRichPresence presence{};
    memset(&presence, 0, sizeof(presence));

    std::vector<std::string> statePool = {
        "ífê¸íÜ // signal: unstable",
        "link lostÅc awaiting null",
        "wired ghost detected",
        "íNÇ‡Ç¢Ç»Ç¢ ? no one is here",
        "network echo: empty",
        "latency error // timeout of self",
        "receiving: static",
        "connectionÅÇreality",
        "reality overlay: corrupted",
        "loop(å«ì∆); // Åá"
    };

    std::vector<std::string> detailPool = {
        "éûä‘écÇË: " + std::to_string(secondsLeft) + " ïb | noise shifting",
        "packets dropped: " + std::to_string(blockedPackets),
        "existence protocol: failing",
        "sys/ping: ?",
        "disconnectingÅc you",
        "trace://self not found",
        "Ç†Ç»ÇΩÇÕíNÅH",
        "404: ego missing",
        "ÉVÉOÉiÉãëré∏íÜ",
        "êMçÜä±è¬ ? signal interference"
    };

    int seed = (std::time(nullptr) / 10) % statePool.size();
    int seed2 = (std::time(nullptr) / 10 + 3) % detailPool.size();

    // Przechowujemy w zmiennych, by c_str() by?o bezpieczne
    std::string currentState = statePool[seed];
    std::string currentDetails = detailPool[seed2];
    std::string smallText = "dropped: " + std::to_string(blockedPackets);


    presence.state = "ífê¸íÜ // signal: unstable";
    presence.details = "packets dropped: 0";
    presence.startTimestamp = time(nullptr);
    presence.largeImageKey = "noise";
    presence.largeImageText = "n0iseífê¸ // disconnect from the real";
    presence.smallImageKey = "ífê¸";

    Discord_UpdatePresence(&presence);
}

// ==================== CONFIG FILE HANDLING ====================
void LoadConfig() {
    std::ifstream configFile("config.ini");
    if (configFile.is_open()) {
        std::string line;
        while (std::getline(configFile, line)) {
            size_t delimiter = line.find('=');
            if (delimiter != std::string::npos) {
                std::string key = line.substr(0, delimiter);
                std::string value = line.substr(delimiter + 1);

                if (key == "activationKey") {
                    config.activationKey = std::stoi(value);
                }
                else if (key == "blockTime") {
                    config.blockTime = std::stoi(value);
                }
                else if (key == "discordRPC") {
                    config.discordRPC = (value == "true" || value == "1");
                }
            }
        }
        configFile.close();
    }
}

void SaveConfig() {
    std::ofstream configFile("config.ini");
    if (configFile.is_open()) {
        configFile << "activationKey=" << config.activationKey << "\n";
        configFile << "blockTime=" << config.blockTime << "\n";
        configFile << "discordRPC=" << (config.discordRPC ? "true" : "false") << "\n";
        configFile.close();
    }
}

// ==================== Sound MP3 ====================

void PlayClickSound() {
    mciSendStringA("open \"click_exec.mp3\" type mpegvideo alias dropSound", nullptr, 0, nullptr);
    mciSendStringA("play dropSound from 0", nullptr, 0, nullptr);
}


// ==================== CONSTANTS & ENUMS ====================
namespace {
    constexpr int DEFAULT_BLOCK_TIME = 10; // seconds
    constexpr int KEY_DEBOUNCE_TIME = 300; // milliseconds
    constexpr int STATUS_UPDATE_INTERVAL = 50; // milliseconds
    constexpr int MAX_PACKET_SIZE = 65536;
}

enum class ConsoleColor {
    Black = 0,
    Blue = 1,
    Green = 2,
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Yellow = 6,
    White = 7,
    Bright = 8,
    Default = 7,
    StatusActive = Green | Bright,
    StatusInactive = Red | Bright,
    Warning = Yellow | Bright,
    Info = Cyan | Bright,
    Error = Red | Bright,
    Header = Magenta | Bright,
    Timer = Yellow | Bright
};

// ==================== GLOBAL VARIABLES ====================
namespace {
    std::atomic<bool> g_block_packets(false);
    std::atomic<bool> g_exit_program(false);
    HANDLE g_divert_handle = INVALID_HANDLE_VALUE;
    std::atomic<int> g_timer(0);
    std::atomic<UINT64> g_blocked_packets(0);
}

// ==================== FUNCTION DECLARATIONS ====================
bool IsAdmin();
void RestartAsAdmin();
void PacketThread();
void TimerThread();
void TogglePacketBlocking();
void Cleanup();
void SetConsoleColor(ConsoleColor color);
void PrintHeader();
void PrintStatus();
void PrintHelp();
void UpdateConsoleTitle();
std::string GetFormattedTime();
void DrawProgressBar(float progress, int width = 50);
void ClearScreenAndShowHeader();

// ==================== HELPER FUNCTIONS ====================
void SetConsoleColor(ConsoleColor color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), static_cast<WORD>(color));
}

std::string GetFormattedTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &in_time_t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%H:%M:%S");
    return ss.str();
}

void DrawProgressBar(float progress, int width) {
    std::cout << "[";
    int pos = static_cast<int>(width * progress);
    for (int i = 0; i < width; ++i) {
        std::cout << (i < pos ? "=" : (i == pos ? ">" : " "));
    }
    std::cout << "] " << int(progress * 100.0) << "%";
}

void UpdateConsoleTitle() {
    std::string title = "n0iseífê¸ [lolikuza.ovh] - STATUS: ";
    title += (g_block_packets ? "ACTIVE" : "INACTIVE");
    if (g_block_packets) {
        title += " | TIME LEFT: " + std::to_string(g_timer) + "s";
        title += " | BLOCKED: " + std::to_string(g_blocked_packets);
    }
    SetConsoleTitleA(title.c_str());
}

void ClearStatusLine() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    COORD coord = { 0, csbi.dwCursorPosition.Y };
    DWORD written;
    FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X, coord, &written);
    SetConsoleCursorPosition(hConsole, coord);
}

void ClearScreenAndShowHeader() {
    system("cls");
    PrintHeader();
    PrintHelp();
}

// ==================== CORE LOGIC ====================
bool IsAdmin() {
    BOOL is_admin = FALSE;
    SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
    PSID admin_group = nullptr;
    if (AllocateAndInitializeSid(&nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &admin_group)) {
        CheckTokenMembership(nullptr, admin_group, &is_admin);
        FreeSid(admin_group);
    }
    return is_admin;
}

void RestartAsAdmin() {
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(nullptr, path, MAX_PATH)) {
        ShellExecuteW(nullptr, L"runas", path, nullptr, nullptr, SW_SHOW);
    }
    std::exit(0);
}

bool OpenDivertHandle() {
    g_divert_handle = WinDivertOpen("outbound", WINDIVERT_LAYER_NETWORK, 0, 0);
    if (g_divert_handle == INVALID_HANDLE_VALUE) {
        SetConsoleColor(ConsoleColor::Error);
        std::cerr << "[" << GetFormattedTime() << "] ERROR: Failed to open WinDivert (Error: " << GetLastError() << ")" << std::endl;
        SetConsoleColor(ConsoleColor::Default);
        return false;
    }
    return true;
}

void PacketThread() {
    unsigned char packet[MAX_PACKET_SIZE];
    WINDIVERT_ADDRESS addr;
    UINT packet_len;
    while (g_block_packets && !g_exit_program) {
        if (WinDivertRecv(g_divert_handle, packet, sizeof(packet), &packet_len, &addr)) {
            g_blocked_packets++;
        }
        UpdateConsoleTitle();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void TimerThread() {
    while (g_timer > 0 && g_block_packets && !g_exit_program) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        g_timer--;
        UpdateConsoleTitle();
    }
    if (g_block_packets) {
        g_block_packets = false;
        SetConsoleColor(ConsoleColor::Warning);
        ClearScreenAndShowHeader();
        std::cout << "[" << GetFormattedTime() << "] [AUTO_PROTOCOL] Data flow restored" << std::endl;
        SetConsoleColor(ConsoleColor::Default);
        PrintStatus();
    }
}

void TogglePacketBlocking() {
    if (!g_block_packets) {
        if (!OpenDivertHandle()) return;
        PlayClickSound();
        g_block_packets = true;
        g_timer = config.blockTime;
        g_blocked_packets = 0;
        std::thread(PacketThread).detach();
        std::thread(TimerThread).detach();
        SetConsoleColor(ConsoleColor::StatusActive);
        ClearStatusLine();
        std::cout << "[" << GetFormattedTime() << "] [SYSTEM] Connection Void: ENGAGED" << std::endl;
    }
    else {
        g_block_packets = false;
        WinDivertClose(g_divert_handle);
        g_divert_handle = INVALID_HANDLE_VALUE;
        ClearScreenAndShowHeader();
        SetConsoleColor(ConsoleColor::StatusInactive);
        std::cout << "[" << GetFormattedTime() << "] [SYSTEM] Connection Void: TERMINATED" << std::endl;
    }
    SetConsoleColor(ConsoleColor::Default);
    UpdateConsoleTitle();
    PrintStatus();
}

void Cleanup() {
    g_exit_program = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (g_divert_handle != INVALID_HANDLE_VALUE) {
        WinDivertClose(g_divert_handle);
        g_divert_handle = INVALID_HANDLE_VALUE;
    }
    g_block_packets = false;
    UpdateConsoleTitle();
}

// ==================== UI ====================
void PrintHeader() {
    SetConsoleColor(ConsoleColor::Header);
    std::cout << R"(
????????????????????????????????????
?????????????????????????????????????
?????????????????????????????????????
?????????????????????????????????????
?????????????????????????????????????
?????????????????????????????????????
?????????????????????????????????????
?????????????????????????????????????
?????????????????????????????????????
?????????????????????????????????????
?????????????????????????????????????
?????????????????????????????????????
?????????????????????????????????????
?????????????????????????????????????
?????????????????????????????????????
??????????????????lolikuza.ovh
)" << std::endl;
    SetConsoleColor(ConsoleColor::Default);
}

void PrintStatus() {
    static bool firstRun = true;
    if (firstRun) {
        firstRun = false;
        return;
    }

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    COORD coord = { 0, csbi.dwCursorPosition.Y };
    SetConsoleCursorPosition(hConsole, coord);

    SetConsoleColor(g_block_packets ? ConsoleColor::StatusActive : ConsoleColor::StatusInactive);
    std::cout << "[" << GetFormattedTime() << "] LAYER_STATUS::" << (g_block_packets ? "\x1b[32m[ACTIVE]\x1b[0m" : "\x1b[31m[STANDBY]\x1b[0m");

    if (g_block_packets) {
        SetConsoleColor(ConsoleColor::Timer);
        std::cout << " || SYNC_DECAY:" << std::setw(2) << g_timer << "s";
        std::cout << " || PACKETS_TERMINATED:" << g_blocked_packets << " ";
        float progress = 1.0f - (static_cast<float>(g_timer) / DEFAULT_BLOCK_TIME);
        DrawProgressBar(progress, 20);
    }

    std::cout << std::flush;
    SetConsoleColor(ConsoleColor::Default);
}

void PrintHelp() {
    SetConsoleColor(ConsoleColor::Info);
    std::cout << "\n[SYSTEM_INTERFACE]" << std::endl;
    std::cout << "  >> CURRENT_ACTIVATION_KEY: " << (config.activationKey == VK_CAPITAL ? "CAPS_LOCK" :
        (config.activationKey == VK_F12 ? "F12" : "VK_" + std::to_string(config.activationKey))) << std::endl;
    std::cout << "  >> BLOCK_TIME: " << config.blockTime << "s" << std::endl;
    std::cout << "  >> SYS_READY" << std::endl;
    SetConsoleColor(ConsoleColor::Default);
}


typedef HANDLE(WINAPI* WinDivertOpen_t)(const char*, INT64, INT, UINT64);
typedef BOOL(WINAPI* WinDivertClose_t)(HANDLE);

// ==================== MAIN ====================
int main() {
    LoadConfig();  // Load configuration at startup

    if (config.discordRPC) {
        InitializeDiscordRPC();
        UpdateDiscordRPC();
    }

    if (!IsAdmin()) {
        SetConsoleColor(ConsoleColor::Warning);
        std::cout << "[" << GetFormattedTime() << "] [WARNING] Elevating to security layer..z." << std::endl;
        SetConsoleColor(ConsoleColor::Default);
        RestartAsAdmin();
        return 0;
    }

    ClearScreenAndShowHeader();
    UpdateConsoleTitle();
    PrintStatus();

    while (!g_exit_program) {
        if (GetAsyncKeyState(config.activationKey) & 0x8000) {  // Use configurable key
            TogglePacketBlocking();
            std::this_thread::sleep_for(std::chrono::milliseconds(KEY_DEBOUNCE_TIME));
        }

        // Check for config reload key (example: F5)
        if (GetAsyncKeyState(VK_F5) & 0x8000) {
            LoadConfig();
            ClearScreenAndShowHeader();
            std::cout << "[" << GetFormattedTime() << "] [SYSTEM] Configuration reloaded" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(KEY_DEBOUNCE_TIME));
        }

        UpdateDiscordRPC(5, 50);
        PrintStatus();
        std::this_thread::sleep_for(std::chrono::milliseconds(STATUS_UPDATE_INTERVAL));
    }

    Cleanup();
    if (config.discordRPC) {
        Discord_Shutdown();
    }

    SetConsoleColor(ConsoleColor::Info);
    std::cout << "\n[" << GetFormattedTime() << "] [SYSTEM] Disconnecting from network layer..." << std::endl;
    SetConsoleColor(ConsoleColor::Default);
    return 0;
}