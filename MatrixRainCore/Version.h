#pragma once

// Version information for MatrixRain
// The build number and year are automatically updated by the pre-build script

#define VERSION_MAJOR 1
#define VERSION_MINOR 1
#define VERSION_BUILD 1868
#define VERSION_YEAR 2026

// Helper macros for stringification
#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

// Full version string as wide string (e.g., L"1.1.1001")
#define VERSION_WSTRING WIDEN(STRINGIFY(VERSION_MAJOR)) L"." WIDEN(STRINGIFY(VERSION_MINOR)) L"." WIDEN(STRINGIFY(VERSION_BUILD))

// Build timestamp as wide string (uses compiler's __DATE__ and __TIME__)
#define VERSION_BUILD_TIMESTAMP WIDEN(__DATE__) L" " WIDEN(__TIME__)

// Current year as wide string (e.g., L"2026")
#define VERSION_YEAR_WSTRING WIDEN(STRINGIFY(VERSION_YEAR))

// Architecture string
#if defined(_M_ARM64)
    #define VERSION_ARCH_WSTRING L"ARM64"
#elif defined(_M_X64) || defined(_M_AMD64)
    #define VERSION_ARCH_WSTRING L"x64"
#else
    #define VERSION_ARCH_WSTRING L"Unknown"
#endif
