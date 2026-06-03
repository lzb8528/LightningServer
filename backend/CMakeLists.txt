cmake_minimum_required(VERSION 3.20)
project(LightningServer VERSION 1.0.0 LANGUAGES CXX)

# Suppress FindBoost deprecation warning (CMake >= 4.0)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# ============================================================================
# C++ standard
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Platform
# ============================================================================
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
endif()

if(PLATFORM_WINDOWS)
    add_compile_definitions(_WIN32_WINNT=0x0A00 NOMINMAX)
endif()

# ============================================================================
# Dependency resolution strategy
# ============================================================================
# Three tiers, checked in this order:
#   1. backend/deps/<name>/include/...                     (run download_deps.ps1)
#   2. -D<NAME>_DIR=/custom/path                            (manual override)
#   3. find_package / system install                        (apt, brew, vcpkg)
#   4. FetchContent (opt-in: -DFETCH_DEPS=ON, needs GitHub)
# ============================================================================

set(DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps" CACHE PATH "Directory for pre-downloaded headers")

option(FETCH_DEPS  "Download missing dependencies from GitHub" OFF)
option(FORCE_MOCK "Force mock DB/cache mode even if MySQL/hiredis are found" OFF)
option(USE_MYSQL  "Link real MySQL client library" OFF)
option(USE_REDIS  "Link real hiredis library"      OFF)

set(SPDLOG_DIR "" CACHE PATH "spdlog include prefix")
set(JSON_DIR   "" CACHE PATH "nlohmann/json include prefix")
set(ASIO_DIR   "" CACHE PATH "standalone Asio source prefix")

# ------------------------------------------------------------------
# spdlog (header-only logging)
# ------------------------------------------------------------------
set(_found_spdlog FALSE)

# Tier 1: deps/
if(NOT _found_spdlog AND EXISTS "${DEPS_DIR}/spdlog/include/spdlog/spdlog.h")
    message(STATUS "spdlog: found in deps/")
    add_library(spdlog_local INTERFACE)
    target_include_directories(spdlog_local INTERFACE "${DEPS_DIR}/spdlog/include")
    target_compile_definitions(spdlog_local INTERFACE SPDLOG_HEADER_ONLY)
    add_library(spdlog::spdlog ALIAS spdlog_local)
    set(_found_spdlog TRUE)
endif()

# Tier 2: user-specified SPDLOG_DIR
if(NOT _found_spdlog AND SPDLOG_DIR AND EXISTS "${SPDLOG_DIR}/include/spdlog/spdlog.h")
    message(STATUS "spdlog: found at ${SPDLOG_DIR}")
    add_library(spdlog_local INTERFACE)
    target_include_directories(spdlog_local INTERFACE "${SPDLOG_DIR}/include")
    target_compile_definitions(spdlog_local INTERFACE SPDLOG_HEADER_ONLY)
    add_library(spdlog::spdlog ALIAS spdlog_local)
    set(_found_spdlog TRUE)
endif()

# Tier 3: find_package
if(NOT _found_spdlog)
    find_package(spdlog QUIET)
    if(spdlog_FOUND)
        message(STATUS "spdlog: found via find_package")
        set(_found_spdlog TRUE)
    endif()
endif()

# Tier 4: FetchContent (opt-in)
if(NOT _found_spdlog AND FETCH_DEPS)
    include(FetchContent)
    FetchContent_Declare(spdlog_fetch
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.14.1
    )
    FetchContent_MakeAvailable(spdlog_fetch)
    if(TARGET spdlog::spdlog)
        set(_found_spdlog TRUE)
    endif()
endif()

if(NOT _found_spdlog)
    message(FATAL_ERROR
        "spdlog not found!\n"
        "  Option A: Run download helper → powershell -ExecutionPolicy Bypass -File download_deps.ps1\n"
        "  Option B: Manual → download & extract https://github.com/gabime/spdlog/archive/refs/tags/v1.14.1.zip\n"
        "             place the include/spdlog/ folder under backend/deps/spdlog/include/spdlog/\n"
        "  Option C: MSYS2 → pacman -S mingw-w64-x86_64-spdlog\n"
        "  Option D: vcpkg → vcpkg install spdlog\n"
    )
endif()

# ------------------------------------------------------------------
# nlohmann/json (header-only JSON)
# ------------------------------------------------------------------
set(_found_json FALSE)

if(NOT _found_json AND EXISTS "${DEPS_DIR}/json/include/nlohmann/json.hpp")
    message(STATUS "nlohmann/json: found in deps/")
    add_library(json_local INTERFACE)
    target_include_directories(json_local INTERFACE "${DEPS_DIR}/json/include")
    add_library(nlohmann_json::nlohmann_json ALIAS json_local)
    set(_found_json TRUE)
endif()

if(NOT _found_json AND JSON_DIR AND EXISTS "${JSON_DIR}/include/nlohmann/json.hpp")
    message(STATUS "nlohmann/json: found at ${JSON_DIR}")
    add_library(json_local INTERFACE)
    target_include_directories(json_local INTERFACE "${JSON_DIR}/include")
    add_library(nlohmann_json::nlohmann_json ALIAS json_local)
    set(_found_json TRUE)
endif()

if(NOT _found_json)
    find_package(nlohmann_json QUIET)
    if(nlohmann_json_FOUND)
        message(STATUS "nlohmann/json: found via find_package")
        set(_found_json TRUE)
    endif()
endif()

if(NOT _found_json AND FETCH_DEPS)
    include(FetchContent)
    FetchContent_Declare(json_fetch
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
    )
    FetchContent_MakeAvailable(json_fetch)
    if(TARGET nlohmann_json::nlohmann_json)
        set(_found_json TRUE)
    endif()
endif()

if(NOT _found_json)
    message(FATAL_ERROR
        "nlohmann/json not found!\n"
        "  Option A: Run download helper → powershell -ExecutionPolicy Bypass -File download_deps.ps1\n"
        "  Option B: Manual → download & extract https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip\n"
        "             place include/nlohmann/json.hpp under backend/deps/json/include/nlohmann/\n"
        "  Option C: MSYS2 → pacman -S mingw-w64-x86_64-nlohmann-json\n"
        "  Option D: vcpkg → vcpkg install nlohmann-json\n"
    )
endif()

# ------------------------------------------------------------------
# Threads
# ------------------------------------------------------------------
find_package(Threads REQUIRED)

# ------------------------------------------------------------------
# Asio — Boost.Asio or standalone Asio
# ------------------------------------------------------------------
set(_found_asio FALSE)

find_package(Boost 1.74 QUIET)
if(Boost_FOUND)
    message(STATUS "Asio: using Boost ${Boost_VERSION}")
    set(ASIO_TARGET Boost::headers)
    if(TARGET Boost::system)
        set(ASIO_LIBS Boost::headers Boost::system)
    else()
        set(ASIO_LIBS Boost::headers)
    endif()
    set(_found_asio TRUE)
endif()

if(NOT _found_asio AND EXISTS "${DEPS_DIR}/asio/asio/include/asio.hpp")
    message(STATUS "Asio: found in deps/")
    add_library(asio_standalone INTERFACE)
    target_include_directories(asio_standalone INTERFACE "${DEPS_DIR}/asio/asio/include")
    target_compile_definitions(asio_standalone INTERFACE ASIO_STANDALONE)
    if(PLATFORM_WINDOWS)
        target_link_libraries(asio_standalone INTERFACE ws2_32 wsock32)
    endif()
    set(ASIO_TARGET asio_standalone)
    set(ASIO_LIBS asio_standalone)
    set(_found_asio TRUE)
endif()

if(NOT _found_asio AND ASIO_DIR AND EXISTS "${ASIO_DIR}/asio/include/asio.hpp")
    message(STATUS "Asio: found at ${ASIO_DIR}")
    add_library(asio_standalone INTERFACE)
    target_include_directories(asio_standalone INTERFACE "${ASIO_DIR}/asio/include")
    target_compile_definitions(asio_standalone INTERFACE ASIO_STANDALONE)
    if(PLATFORM_WINDOWS)
        target_link_libraries(asio_standalone INTERFACE ws2_32 wsock32)
    endif()
    set(ASIO_TARGET asio_standalone)
    set(ASIO_LIBS asio_standalone)
    set(_found_asio TRUE)
endif()

if(NOT _found_asio AND FETCH_DEPS)
    include(FetchContent)
    FetchContent_Declare(asio_fetch
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG        asio-1-30-2
        GIT_SHALLOW    TRUE
    )
    FetchContent_GetProperties(asio_fetch)
    if(NOT asio_fetch_POPULATED)
        FetchContent_Populate(asio_fetch)
    endif()
    if(EXISTS "${asio_fetch_SOURCE_DIR}/asio/include/asio.hpp")
        add_library(asio_standalone INTERFACE)
        target_include_directories(asio_standalone INTERFACE "${asio_fetch_SOURCE_DIR}/asio/include")
        target_compile_definitions(asio_standalone INTERFACE ASIO_STANDALONE)
        if(PLATFORM_WINDOWS)
            target_link_libraries(asio_standalone INTERFACE ws2_32 wsock32)
        endif()
        set(ASIO_TARGET asio_standalone)
        set(ASIO_LIBS asio_standalone)
        set(_found_asio TRUE)
    endif()
endif()

if(NOT _found_asio)
    message(FATAL_ERROR
        "Asio (networking library) not found!\n"
        "  Option A: Run download helper → powershell -ExecutionPolicy Bypass -File download_deps.ps1\n"
        "  Option B: Manual → download & extract https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-30-2.zip\n"
        "             place the asio/ folder under backend/deps/asio/asio/\n"
        "  Option C: MSYS2 → pacman -S mingw-w64-x86_64-boost\n"
        "  Option D: vcpkg → vcpkg install boost-asio\n"
    )
endif()

# ============================================================================
# MySQL / hiredis (optional — mock mode if not found)
# ============================================================================
find_path(MYSQL_INCLUDE_DIR mysql/mysql.h
    PATHS
        /usr/include /usr/local/include /usr/include/mysql
        /usr/local/opt/mysql-client/include
        "C:/Program Files/MySQL/MySQL Server 8.0/include"
        "C:/Program Files/MySQL/MySQL Server 8.4/include"
        "C:/Program Files/MySQL/MySQL Server 9.0/include"
        "$ENV{VCPKG_ROOT}/installed/x64-windows/include"
        "$ENV{MYSQL_DIR}/include"
)

find_library(MYSQL_LIBRARY
    NAMES mysqlclient libmysql libmariadb
    PATHS
        /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
        /usr/local/opt/mysql-client/lib
        "C:/Program Files/MySQL/MySQL Server 8.0/lib"
        "C:/Program Files/MySQL/MySQL Server 8.4/lib"
        "C:/Program Files/MySQL/MySQL Server 9.0/lib"
        "$ENV{VCPKG_ROOT}/installed/x64-windows/lib"
        "$ENV{MYSQL_DIR}/lib"
)

find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h
    PATHS
        /usr/include /usr/local/include
        /usr/local/opt/hiredis/include
        "$ENV{VCPKG_ROOT}/installed/x64-windows/include"
        "$ENV{HIREDIS_DIR}/include"
)

find_library(HIREDIS_LIBRARY
    NAMES hiredis
    PATHS
        /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
        /usr/local/opt/hiredis/lib
        "$ENV{VCPKG_ROOT}/installed/x64-windows/lib"
        "$ENV{HIREDIS_DIR}/lib"
)

# ============================================================================
# Source files
# ============================================================================
set(BACKEND_SOURCES
    src/main.cpp
    src/net/HttpParser.cpp
    src/net/HttpServer.cpp
    src/net/HttpSession.cpp
    src/db/DatabaseProxy.cpp
    src/cache/CacheProxy.cpp
    src/service/ArticleService.cpp
    src/service/CommentService.cpp
    src/handler/ApiHandler.cpp
    src/util/Config.cpp
)

# ============================================================================
# Executable
# ============================================================================
add_executable(lightning_server ${BACKEND_SOURCES})

# Ensure local deps/ headers take priority over system/vcpkg installations
target_include_directories(lightning_server BEFORE PRIVATE
    ${DEPS_DIR}/spdlog/include
    ${DEPS_DIR}/json/include
    ${DEPS_DIR}/asio/asio/include
)
# Force spdlog header-only mode (uses bundled fmt, avoids linking external fmt library)
target_compile_definitions(lightning_server PRIVATE SPDLOG_HEADER_ONLY)

target_include_directories(lightning_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(lightning_server PRIVATE
    Threads::Threads
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    ${ASIO_LIBS}
)

# ============================================================================
# MySQL / hiredis
# ============================================================================
if(USE_MYSQL AND NOT FORCE_MOCK AND MYSQL_LIBRARY AND MYSQL_INCLUDE_DIR)
    target_include_directories(lightning_server SYSTEM PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(lightning_server PRIVATE ${MYSQL_LIBRARY})
    target_compile_definitions(lightning_server PRIVATE HAS_MYSQL=1)
    message(STATUS "MySQL:  ${MYSQL_LIBRARY}")
else()
    target_compile_definitions(lightning_server PRIVATE MOCK_DB=1)
    if(FORCE_MOCK)
        message(STATUS "MySQL:  mock mode (forced)")
    else()
        message(STATUS "MySQL:  mock mode (not found)")
    endif()
endif()

if(USE_REDIS AND NOT FORCE_MOCK AND HIREDIS_LIBRARY AND HIREDIS_INCLUDE_DIR)
    target_include_directories(lightning_server SYSTEM PRIVATE ${HIREDIS_INCLUDE_DIR})
    target_link_libraries(lightning_server PRIVATE ${HIREDIS_LIBRARY})
    target_compile_definitions(lightning_server PRIVATE HAS_REDIS=1)
    message(STATUS "Redis:  ${HIREDIS_LIBRARY}")
else()
    target_compile_definitions(lightning_server PRIVATE MOCK_CACHE=1)
    if(FORCE_MOCK)
        message(STATUS "Redis:  mock mode (forced)")
    else()
        message(STATUS "Redis:  mock mode (not found)")
    endif()
endif()

# ============================================================================
# Compiler flags
# ============================================================================
if(MSVC)
    target_compile_options(lightning_server PRIVATE /W4 /O2 /utf-8)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(lightning_server PRIVATE -Wall -Wextra -O2 -D_FORTIFY_SOURCE=2)
    if(PLATFORM_LINUX OR PLATFORM_MACOS)
        set_target_properties(lightning_server PROPERTIES
            INSTALL_RPATH "\$ORIGIN"
            BUILD_RPATH "\$ORIGIN"
        )
    endif()
endif()

# ============================================================================
# Install
# ============================================================================
install(TARGETS lightning_server DESTINATION bin)

# ============================================================================
# Summary
# ============================================================================
message(STATUS "")
message(STATUS "=== LightningServer Build Configuration ===")
message(STATUS "Platform:  ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Compiler:  ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build:     ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ std:   ${CMAKE_CXX_STANDARD}")
message(STATUS "============================================")
message(STATUS "")
