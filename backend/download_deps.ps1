# ============================================================================
# LightningServer - Dependency Downloader (Windows PowerShell)
# ============================================================================
# Downloads header-only C++ dependencies and places them under backend/deps/
# so CMake can build without network / FetchContent.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File download_deps.ps1
# ============================================================================

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$DepsDir   = Join-Path $ScriptDir "deps"
$TempDir   = Join-Path $ScriptDir "deps_tmp"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host " LightningServer - Download Dependencies" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

if (Test-Path $TempDir) { Remove-Item -Recurse -Force $TempDir }
New-Item -ItemType Directory -Force -Path $TempDir | Out-Null
New-Item -ItemType Directory -Force -Path $DepsDir | Out-Null

# ------------------------------------------------------------------
# spdlog v1.14.1
# ------------------------------------------------------------------
$SPDLOG_VER = "v1.14.1"
$SPDLOG_DIR = Join-Path $DepsDir "spdlog"
$SPDLOG_HDR = Join-Path $SPDLOG_DIR "include\spdlog\spdlog.h"

if (Test-Path $SPDLOG_HDR) {
    Write-Host "[OK] spdlog already exists" -ForegroundColor Green
} else {
    Write-Host "[INFO] Downloading spdlog $SPDLOG_VER ..." -ForegroundColor Yellow
    $Url = "https://github.com/gabime/spdlog/archive/refs/tags/$SPDLOG_VER.zip"
    $Zip = Join-Path $TempDir "spdlog.zip"
    $OK = $false

    try {
        Invoke-WebRequest -Uri $Url -OutFile $Zip -TimeoutSec 60
        $OK = $true
    } catch {
        Write-Host "[WARN] GitHub failed, trying gitee mirror..." -ForegroundColor Yellow
    }

    if (-not $OK) {
        $Url = "https://gitee.com/mirrors/spdlog/repository/archive/$SPDLOG_VER.zip"
        try {
            Invoke-WebRequest -Uri $Url -OutFile $Zip -TimeoutSec 60
            $OK = $true
        } catch {
            Write-Host "[WARN] Gitee mirror also failed" -ForegroundColor Yellow
        }
    }

    if ($OK -and (Test-Path $Zip)) {
        Expand-Archive -Path $Zip -DestinationPath $TempDir -Force
        $Extracted = Get-ChildItem -Path $TempDir -Directory | Where-Object { $_.Name -like "spdlog*" } | Select-Object -First 1
        $IncludeDir = Join-Path $SPDLOG_DIR "include"
        New-Item -ItemType Directory -Force -Path $IncludeDir | Out-Null
        Copy-Item -Recurse -Force (Join-Path $Extracted.FullName "include\spdlog") (Join-Path $IncludeDir "spdlog")
        Write-Host "[OK] spdlog installed" -ForegroundColor Green
    } else {
        Write-Host "[ERROR] Cannot download spdlog. Manual steps:" -ForegroundColor Red
        Write-Host "        Download: https://github.com/gabime/spdlog/archive/refs/tags/v1.14.1.zip" -ForegroundColor Red
        Write-Host "        Extract include/spdlog/ to: $SPDLOG_DIR\include\spdlog\" -ForegroundColor Red
    }
}

# ------------------------------------------------------------------
# nlohmann/json v3.11.3
# ------------------------------------------------------------------
$JSON_VER = "v3.11.3"
$JSON_DIR = Join-Path $DepsDir "json"
$JSON_HDR = Join-Path $JSON_DIR "include\nlohmann\json.hpp"

if (Test-Path $JSON_HDR) {
    Write-Host "[OK] nlohmann/json already exists" -ForegroundColor Green
} else {
    Write-Host "[INFO] Downloading nlohmann/json $JSON_VER ..." -ForegroundColor Yellow
    $Url = "https://github.com/nlohmann/json/archive/refs/tags/$JSON_VER.zip"
    $Zip = Join-Path $TempDir "json.zip"
    $OK = $false

    try {
        Invoke-WebRequest -Uri $Url -OutFile $Zip -TimeoutSec 60
        $OK = $true
    } catch {
        Write-Host "[WARN] GitHub failed, trying gitee mirror..." -ForegroundColor Yellow
    }

    if (-not $OK) {
        $Url = "https://gitee.com/mirrors/json/repository/archive/$JSON_VER.zip"
        try {
            Invoke-WebRequest -Uri $Url -OutFile $Zip -TimeoutSec 60
            $OK = $true
        } catch {
            Write-Host "[WARN] Gitee mirror also failed" -ForegroundColor Yellow
        }
    }

    if ($OK -and (Test-Path $Zip)) {
        Expand-Archive -Path $Zip -DestinationPath $TempDir -Force
        $Extracted = Get-ChildItem -Path $TempDir -Directory | Where-Object { $_.Name -like "json*" } | Select-Object -First 1
        $NlohmannDir = Join-Path $JSON_DIR "include\nlohmann"
        New-Item -ItemType Directory -Force -Path $NlohmannDir | Out-Null
        $SingleHdr = Join-Path $Extracted.FullName "single_include\nlohmann\json.hpp"
        if (Test-Path $SingleHdr) {
            Copy-Item -Force $SingleHdr (Join-Path $NlohmannDir "json.hpp")
        } else {
            Copy-Item -Recurse -Force (Join-Path $Extracted.FullName "include\nlohmann") $NlohmannDir
        }
        Write-Host "[OK] nlohmann/json installed" -ForegroundColor Green
    } else {
        Write-Host "[ERROR] Cannot download json. Manual steps:" -ForegroundColor Red
        Write-Host "        Download: https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip" -ForegroundColor Red
        Write-Host "        Extract to: $JSON_DIR\include\" -ForegroundColor Red
    }
}

# ------------------------------------------------------------------
# Asio v1.30.2
# ------------------------------------------------------------------
$ASIO_VER = "asio-1-30-2"
$ASIO_DIR = Join-Path $DepsDir "asio"
$ASIO_HDR = Join-Path $ASIO_DIR "asio\include\asio.hpp"

if (Test-Path $ASIO_HDR) {
    Write-Host "[OK] Asio already exists" -ForegroundColor Green
} else {
    Write-Host "[INFO] Downloading standalone Asio $ASIO_VER ..." -ForegroundColor Yellow
    $Url = "https://github.com/chriskohlhoff/asio/archive/refs/tags/$ASIO_VER.zip"
    $Zip = Join-Path $TempDir "asio.zip"
    $OK = $false

    try {
        Invoke-WebRequest -Uri $Url -OutFile $Zip -TimeoutSec 60
        $OK = $true
    } catch {
        Write-Host "[WARN] GitHub failed, trying gitee mirror..." -ForegroundColor Yellow
    }

    if (-not $OK) {
        $Url = "https://gitee.com/mirrors/asio/repository/archive/$ASIO_VER.zip"
        try {
            Invoke-WebRequest -Uri $Url -OutFile $Zip -TimeoutSec 60
            $OK = $true
        } catch {
            Write-Host "[WARN] Gitee mirror also failed" -ForegroundColor Yellow
        }
    }

    if ($OK -and (Test-Path $Zip)) {
        Expand-Archive -Path $Zip -DestinationPath $TempDir -Force
        $Extracted = Get-ChildItem -Path $TempDir -Directory | Where-Object { $_.Name -like "asio*" } | Select-Object -First 1
        $AsioSubDir = Join-Path $ASIO_DIR "asio"
        New-Item -ItemType Directory -Force -Path $AsioSubDir | Out-Null
        Copy-Item -Recurse -Force (Join-Path $Extracted.FullName "asio\include") (Join-Path $AsioSubDir "include")
        Write-Host "[OK] Asio installed" -ForegroundColor Green
    } else {
        Write-Host "[ERROR] Cannot download Asio. Manual steps:" -ForegroundColor Red
        Write-Host "        Download: https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-30-2.zip" -ForegroundColor Red
        Write-Host "        Extract to: $ASIO_DIR\asio\" -ForegroundColor Red
    }
}

# ------------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------------
if (Test-Path $TempDir) { Remove-Item -Recurse -Force $TempDir }

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Done! Now run:  cd backend && build.bat" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
