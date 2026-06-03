@echo off
setlocal

echo ============================================
echo  LightningServer - Windows Build
echo ============================================
echo.

REM ------------------------------------------------------------------
REM Check: are dependencies present?
REM ------------------------------------------------------------------
set DEPS_OK=1
if not exist "deps\spdlog\include\spdlog\spdlog.h"  set DEPS_OK=0
if not exist "deps\json\include\nlohmann\json.hpp" set DEPS_OK=0
if not exist "deps\asio\asio\include\asio.hpp"    set DEPS_OK=0

if %DEPS_OK% equ 0 (
    echo [WARN] Dependencies not found in backend\deps\
    echo.
    echo The build needs 3 header-only libraries: spdlog, nlohmann/json, Asio.
    echo They will be downloaded automatically if GitHub is accessible,
    echo but your network seems to block GitHub.
    echo.
    echo Option 1: Download them now via PowerShell:
    echo   powershell -ExecutionPolicy Bypass -File download_deps.ps1
    echo.
    echo Option 2: Download manually and place under backend\deps\:
    echo   backend\deps\spdlog\include\spdlog\spdlog.h
    echo   backend\deps\json\include\nlohmann\json.hpp
    echo   backend\deps\asio\asio\include\asio.hpp
    echo.
    echo Option 3: Install via MSYS2/MinGW package manager:
    echo   pacman -S mingw-w64-x86_64-spdlog mingw-w64-x86_64-nlohmann-json
    echo.
    set /p CHOICE="Run download script now? [Y/n]: "
    if /i not "%CHOICE%"=="n" (
        echo.
        powershell -ExecutionPolicy Bypass -File download_deps.ps1
    )
)

REM ------------------------------------------------------------------
REM Detect available generator
REM ------------------------------------------------------------------
set GENERATOR=
set GENERATOR_NAME=
set BUILD_CMD=cmake --build . --config Release

REM 1. Ninja
where ninja >nul 2>&1
if %errorlevel% equ 0 (
    echo [INFO] Generator: Ninja
    set GENERATOR=Ninja
    set GENERATOR_NAME=Ninja
    goto :found
)

REM 2. Visual Studio 2022
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    echo [INFO] Generator: Visual Studio 2022 Enterprise
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    set "GENERATOR=Visual Studio 17 2022 -A x64"
    goto :found
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    echo [INFO] Generator: Visual Studio 2022 Professional
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    set "GENERATOR=Visual Studio 17 2022 -A x64"
    goto :found
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    echo [INFO] Generator: Visual Studio 2022 Community
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    set "GENERATOR=Visual Studio 17 2022 -A x64"
    goto :found
)

REM 3. MinGW
where mingw32-make >nul 2>&1
if %errorlevel% equ 0 (
    echo [INFO] Generator: MinGW
    set "GENERATOR=MinGW Makefiles"
    goto :found
)

REM 4. Fallback
echo [INFO] Generator: CMake default
:found

REM ------------------------------------------------------------------
REM Clean + Build
REM ------------------------------------------------------------------
if exist build (
    echo [INFO] Removing stale build directory...
    rmdir /s /q build
)

mkdir build
cd build

echo [INFO] Configuring CMake...
if defined GENERATOR (
    cmake .. -G "%GENERATOR%" -DCMAKE_BUILD_TYPE=Release
) else (
    cmake .. -DCMAKE_BUILD_TYPE=Release
)

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed.
    echo If dependencies are missing, run: powershell -ExecutionPolicy Bypass -File download_deps.ps1
    exit /b 1
)

echo.
echo [INFO] Building...
%BUILD_CMD%

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed
    exit /b 1
)

echo.
echo ============================================
echo  Build succeeded
echo ============================================
echo.

if exist "Release\lightning_server.exe" (
    echo Binary: %cd%\Release\lightning_server.exe
) else if exist "lightning_server.exe" (
    echo Binary: %cd%\lightning_server.exe
) else if exist "Debug\lightning_server.exe" (
    echo Binary: %cd%\Debug\lightning_server.exe
) else (
    echo Binary: lightning_server.exe
    dir /s /b lightning_server.exe 2>nul
)
echo.
echo Run it with:
echo   set LS_HOST=0.0.0.0
echo   set LS_PORT=8080
echo   lightning_server.exe
echo.
