#pragma once

/**
 * @file Platform.h
 * @brief Cross-platform compatibility layer.
 *
 * Provides unified wrappers for OS-specific APIs so business logic stays clean.
 *
 * Windows (MSVC / MinGW / Clang)  → _WIN32
 * Linux   (GCC / Clang)            → __linux__
 * macOS   (Clang)                   → __APPLE__
 */

#include <ctime>
#include <cstdlib>
#include <string>

namespace lightning::platform {

// ---------------------------------------------------------------------------
// thread-safe localtime
// ---------------------------------------------------------------------------
// On Windows:  localtime_s(&tm, &time)   — tm first, then time
// On POSIX:    localtime_r(&time, &tm)   — time first, then tm
// This wrapper always takes (time_t, tm*) in that order and fills *tm.
// ---------------------------------------------------------------------------
inline bool safeLocaltime(const std::time_t* time, std::tm* out) {
    if (!time || !out) return false;
#if defined(_WIN32)
    return ::localtime_s(out, time) == 0;
#else
    return ::localtime_r(time, out) != nullptr;
#endif
}

// ---------------------------------------------------------------------------
// thread-safe getenv
// ---------------------------------------------------------------------------
// getenv() returns a pointer to static storage shared across threads.
// On POSIX it is NOT thread-safe if any thread calls setenv()/putenv().
// On Windows, _dupenv_s allocates a caller-owned buffer (safer).
// ---------------------------------------------------------------------------
inline std::string safeGetenv(const char* key) {
#if defined(_MSC_VER)
    // MSVC: _dupenv_s allocates caller-owned buffer (thread-safe)
    char* buf = nullptr;
    std::size_t len = 0;
    if (::_dupenv_s(&buf, &len, key) == 0 && buf != nullptr) {
        std::string result(buf, len);
        std::free(buf);
        return result;
    }
    return {};
#else
    // MinGW / GCC / Clang: plain getenv (no setenv in our code, so it's safe)
    const char* val = ::getenv(key);
    return val ? std::string(val) : std::string();
#endif
}

inline std::string safeGetenv(const char* key, const std::string& defaultVal) {
    auto val = safeGetenv(key);
    return val.empty() ? defaultVal : val;
}

inline int safeGetenvInt(const char* key, int defaultVal) {
    auto val = safeGetenv(key);
    if (val.empty()) return defaultVal;
    try {
        return std::stoi(val);
    } catch (...) {
        return defaultVal;
    }
}

// ---------------------------------------------------------------------------
// Platform identification (compile-time)
// ---------------------------------------------------------------------------
#if defined(_WIN32)
constexpr bool kIsWindows = true;
constexpr bool kIsLinux   = false;
constexpr bool kIsMacOS   = false;
#elif defined(__APPLE__)
constexpr bool kIsWindows = false;
constexpr bool kIsLinux   = false;
constexpr bool kIsMacOS   = true;
#elif defined(__linux__)
constexpr bool kIsWindows = false;
constexpr bool kIsLinux   = true;
constexpr bool kIsMacOS   = false;
#else
constexpr bool kIsWindows = false;
constexpr bool kIsLinux   = false;
constexpr bool kIsMacOS   = false;
#endif

} // namespace lightning::platform
