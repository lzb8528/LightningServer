#!/usr/bin/env bash
set -euo pipefail

# ================================================================
#  LightningServer — Linux / macOS Build Script
# ================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

info()  { echo -e "${CYAN}[INFO]${NC}  $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
err()   { echo -e "${RED}[ERROR]${NC} $*"; }
ok()    { echo -e "${GREEN}[OK]${NC}    $*"; }

echo ""
echo "============================================"
echo " LightningServer — Build"
echo "============================================"
echo ""

# ------------------------------------------------------------------
# Working directory
# ------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# ------------------------------------------------------------------
# Tool checks
# ------------------------------------------------------------------
check_tool() {
    local tool="$1"
    local hint="${2:-}"
    if ! command -v "$tool" &>/dev/null; then
        err "$tool not found${hint:+ — $hint}"
        MISSING=1
    else
        ok "$tool ($(command -v "$tool"))"
    fi
}

MISSING=0
info "Checking build tools..."
check_tool cmake    "install: sudo apt install cmake  (or brew install cmake)"
check_tool git      "install: sudo apt install git"
check_tool g++      "install: sudo apt install g++"
check_tool make     "install: sudo apt install make"

# Optional
if command -v ninja &>/dev/null; then
    ok "ninja $(command -v ninja)  (will use for faster builds)"
    USE_NINJA=1
else
    info "ninja not found — using make (install ninja for faster builds)"
    USE_NINJA=0
fi

if [ "${MISSING:-0}" -eq 1 ]; then
    echo ""
    err "Please install missing tools and re-run."
    exit 1
fi

# ------------------------------------------------------------------
# Optional dependency hints
# ------------------------------------------------------------------
echo ""
info "Checking optional dependencies..."

MYSQL_OK=0
HIREDIS_OK=0

# Check for mysql_config (the reliable way)
if command -v mysql_config &>/dev/null; then
    ok "MySQL client found ($(mysql_config --version 2>/dev/null || echo 'version unknown'))"
    MYSQL_OK=1
elif pkg-config --exists mysqlclient 2>/dev/null; then
    ok "MySQL client found (pkg-config)"
    MYSQL_OK=1
elif [ -f /usr/include/mysql/mysql.h ] || [ -f /usr/local/include/mysql/mysql.h ]; then
    ok "MySQL headers found"
    MYSQL_OK=1
else
    warn "MySQL development libraries not found — will build with mock DB"
    warn "  Install: sudo apt install libmysqlclient-dev"
    warn "        or: brew install mysql-client"
fi

if command -v hiredis-test &>/dev/null || pkg-config --exists hiredis 2>/dev/null; then
    ok "hiredis found"
    HIREDIS_OK=1
elif [ -f /usr/include/hiredis/hiredis.h ] || [ -f /usr/local/include/hiredis/hiredis.h ]; then
    ok "hiredis headers found"
    HIREDIS_OK=1
else
    warn "hiredis not found — will build with mock cache"
    warn "  Install: sudo apt install libhiredis-dev"
    warn "        or: brew install hiredis"
fi

# ------------------------------------------------------------------
# Configure
# ------------------------------------------------------------------
echo ""
info "Configuring CMake..."

mkdir -p build
cd build

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"

if [ "$USE_NINJA" -eq 1 ]; then
    CMAKE_ARGS="$CMAKE_ARGS -G Ninja"
fi

cmake .. $CMAKE_ARGS

# ------------------------------------------------------------------
# Build
# ------------------------------------------------------------------
echo ""
info "Building..."

# Use all available cores
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
cmake --build . --config Release -j "$NPROC"

# ------------------------------------------------------------------
# Success
# ------------------------------------------------------------------
echo ""
echo "============================================"
echo -e " ${GREEN}Build succeeded!${NC}"
echo "============================================"
echo ""
echo " Binary: $(pwd)/lightning_server"
echo " Run:    ./build/lightning_server"
echo ""
echo " Environment variables:"
echo "   export LS_HOST=0.0.0.0"
echo "   export LS_PORT=8080"
echo "   export LS_THREAD_NUM=4"
echo "   export MYSQL_HOST=127.0.0.1"
echo "   export MYSQL_PORT=3306"
echo "   export MYSQL_USER=lightning"
echo "   export MYSQL_PASSWORD=lightning123"
echo "   export MYSQL_DATABASE=lightning_db"
echo "   export REDIS_HOST=127.0.0.1"
echo "   export REDIS_PORT=6379"
echo ""
