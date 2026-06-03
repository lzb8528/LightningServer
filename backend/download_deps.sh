#!/usr/bin/env bash
# ============================================================================
# LightningServer - Dependency Downloader (Linux / macOS)
# ============================================================================
# Downloads header-only C++ dependencies and places them under backend/deps/
# so CMake can build without network / FetchContent.
#
# Usage:
#   chmod +x download_deps.sh && ./download_deps.sh
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEPS_DIR="$SCRIPT_DIR/deps"
TEMP_DIR="$SCRIPT_DIR/deps_tmp"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'
info()  { echo -e "${CYAN}[INFO]${NC} $*"; }
ok()    { echo -e "${GREEN}[OK]${NC}   $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
err()   { echo -e "${RED}[ERROR]${NC}$*"; }

echo ""
echo "============================================"
echo " LightningServer - Download Dependencies"
echo "============================================"
echo ""

rm -rf "$TEMP_DIR"
mkdir -p "$TEMP_DIR" "$DEPS_DIR"

# Detect download tool
if command -v curl &>/dev/null; then
    DOWNLOAD="curl -L -o"
elif command -v wget &>/dev/null; then
    DOWNLOAD="wget -O"
else
    err "Neither curl nor wget found. Install one first."
    exit 1
fi

# ------------------------------------------------------------------
# spdlog v1.14.1
# ------------------------------------------------------------------
SPDLOG_VER="v1.14.1"
SPDLOG_DIR="$DEPS_DIR/spdlog"
SPDLOG_HDR="$SPDLOG_DIR/include/spdlog/spdlog.h"

if [ -f "$SPDLOG_HDR" ]; then
    ok "spdlog already exists"
else
    info "Downloading spdlog $SPDLOG_VER ..."
    SPDLOG_URL="https://github.com/gabime/spdlog/archive/refs/tags/${SPDLOG_VER}.tar.gz"
    if ! $DOWNLOAD "$TEMP_DIR/spdlog.tar.gz" "$SPDLOG_URL" 2>/dev/null; then
        warn "GitHub failed, trying mirror..."
        SPDLOG_URL="https://gitee.com/mirrors/spdlog/repository/archive/${SPDLOG_VER}.tar.gz"
        $DOWNLOAD "$TEMP_DIR/spdlog.tar.gz" "$SPDLOG_URL" || {
            err "Cannot download spdlog. Manual: download from $SPDLOG_URL"
            err "  Extract to: $SPDLOG_DIR"
        }
    fi
    if [ -f "$TEMP_DIR/spdlog.tar.gz" ]; then
        tar -xzf "$TEMP_DIR/spdlog.tar.gz" -C "$TEMP_DIR"
        EXTRACTED=$(find "$TEMP_DIR" -maxdepth 1 -type d -name "spdlog*" | head -1)
        mkdir -p "$SPDLOG_DIR/include"
        cp -r "$EXTRACTED/include/spdlog" "$SPDLOG_DIR/include/"
        ok "spdlog installed"
    fi
fi

# ------------------------------------------------------------------
# nlohmann/json v3.11.3
# ------------------------------------------------------------------
JSON_VER="v3.11.3"
JSON_DIR="$DEPS_DIR/json"
JSON_HDR="$JSON_DIR/include/nlohmann/json.hpp"

if [ -f "$JSON_HDR" ]; then
    ok "nlohmann/json already exists"
else
    info "Downloading nlohmann/json $JSON_VER ..."
    JSON_URL="https://github.com/nlohmann/json/archive/refs/tags/${JSON_VER}.tar.gz"
    if ! $DOWNLOAD "$TEMP_DIR/json.tar.gz" "$JSON_URL" 2>/dev/null; then
        warn "GitHub failed, trying mirror..."
        JSON_URL="https://gitee.com/mirrors/json/repository/archive/${JSON_VER}.tar.gz"
        $DOWNLOAD "$TEMP_DIR/json.tar.gz" "$JSON_URL" || {
            err "Cannot download json. Manual: download from $JSON_URL"
        }
    fi
    if [ -f "$TEMP_DIR/json.tar.gz" ]; then
        tar -xzf "$TEMP_DIR/json.tar.gz" -C "$TEMP_DIR"
        EXTRACTED=$(find "$TEMP_DIR" -maxdepth 1 -type d -name "json*" | head -1)
        mkdir -p "$JSON_DIR/include/nlohmann"
        if [ -f "$EXTRACTED/single_include/nlohmann/json.hpp" ]; then
            cp "$EXTRACTED/single_include/nlohmann/json.hpp" "$JSON_DIR/include/nlohmann/"
        else
            cp -r "$EXTRACTED/include/nlohmann" "$JSON_DIR/include/"
        fi
        ok "nlohmann/json installed"
    fi
fi

# ------------------------------------------------------------------
# Asio v1.30.2
# ------------------------------------------------------------------
ASIO_VER="asio-1-30-2"
ASIO_DIR="$DEPS_DIR/asio"
ASIO_HDR="$ASIO_DIR/asio/include/asio.hpp"

if [ -f "$ASIO_HDR" ]; then
    ok "Asio already exists"
else
    info "Downloading standalone Asio $ASIO_VER ..."
    ASIO_URL="https://github.com/chriskohlhoff/asio/archive/refs/tags/${ASIO_VER}.tar.gz"
    if ! $DOWNLOAD "$TEMP_DIR/asio.tar.gz" "$ASIO_URL" 2>/dev/null; then
        warn "GitHub failed, trying mirror..."
        ASIO_URL="https://gitee.com/mirrors/asio/repository/archive/${ASIO_VER}.tar.gz"
        $DOWNLOAD "$TEMP_DIR/asio.tar.gz" "$ASIO_URL" || {
            err "Cannot download Asio. Manual: download from $ASIO_URL"
        }
    fi
    if [ -f "$TEMP_DIR/asio.tar.gz" ]; then
        tar -xzf "$TEMP_DIR/asio.tar.gz" -C "$TEMP_DIR"
        EXTRACTED=$(find "$TEMP_DIR" -maxdepth 1 -type d -name "asio*" | head -1)
        mkdir -p "$ASIO_DIR/asio"
        cp -r "$EXTRACTED/asio/include" "$ASIO_DIR/asio/"
        ok "Asio installed"
    fi
fi

# ------------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------------
rm -rf "$TEMP_DIR"

echo ""
echo "============================================"
echo -e " ${GREEN}Done! You can now build with:${NC}"
echo "   cd backend && ./build.sh"
echo "============================================"
echo ""
