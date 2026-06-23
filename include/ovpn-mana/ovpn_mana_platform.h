#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #define OVPN_PLATFORM_WINDOWS
#elif defined(__linux__)
    #define OVPN_PLATFORM_LINUX
#elif defined(__APPLE__)
    #define OVPN_PLATFORM_MACOS
#else
    #error "Unsupported platform"
#endif

#ifdef OVPN_PLATFORM_WINDOWS
    #define OVPN_POPEN  _popen
    #define OVPN_PCLOSE _pclose
#else
    #define OVPN_POPEN  popen
    #define OVPN_PCLOSE pclose
#endif

#ifdef OVPN_PLATFORM_WINDOWS
    #define OVPN_GETUID() 0
#else
    #include <unistd.h>
    #define OVPN_GETUID() getuid()
#endif

#ifdef OVPN_PLATFORM_WINDOWS
    constexpr char OVPN_PATH_SEP = '\\';
#else
    constexpr char OVPN_PATH_SEP = '/';
#endif

#ifdef OVPN_PLATFORM_WINDOWS
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    inline void ovpn_enable_ansi_terminal() {
        SetConsoleOutputCP(CP_UTF8);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#else
    inline void ovpn_enable_ansi_terminal() {}
#endif