#!/bin/bash
# ============================================================
# launcher.sh — Master launcher for ChessBot
# 1. Updates Stockfish to latest version
# 2. Ensures RKKDR kernel driver is loaded
# 3. Launches the chess bot
# ============================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Dynamically detect if we are in the monorepo or standalone
if [ -f "../../src/main.c" ]; then
    DRIVER_DIR="${DRIVER_DIR:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
    BIN="$SCRIPT_DIR/../../release/ChessBot"
else
    DRIVER_DIR="RKKDR"
    BIN="$SCRIPT_DIR/release/ChessBot"
fi

CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  ChessBot Launcher${NC}"
echo -e "${CYAN}========================================${NC}"

# Step 1: Update Stockfish
echo ""
bash "$SCRIPT_DIR/scripts/update_stockfish.sh"

# Step 2: Ensure RKKDR kernel driver is loaded
echo ""
echo -e "${CYAN}[ChessBot] Checking RKKDR kernel driver...${NC}"

if lsmod | grep -q "RKKDR"; then
    echo -e "${GREEN}[ChessBot] RKKDR driver already loaded.${NC}"
else
    echo -e "${YELLOW}[ChessBot] RKKDR driver not loaded. Building and loading...${NC}"
    if [ -d "$DRIVER_DIR" ]; then
        make -C "$DRIVER_DIR" load
        echo -e "${GREEN}[ChessBot] RKKDR driver loaded successfully.${NC}"
    else
        echo -e "${RED}[ChessBot] ERROR: KernelDriver directory not found at $DRIVER_DIR${NC}"
        echo -e "${RED}[ChessBot] Please load the RKKDR driver manually.${NC}"
        exit 1
    fi
fi

# Verify device nodes exist
echo -e "${CYAN}[ChessBot] Verifying device nodes...${NC}"
for dev in /dev/rkkdr_mouse /dev/rkkdr_keyboard /dev/rkkdr_screen; do
    if [ -e "$dev" ]; then
        echo -e "${GREEN}  ✓ $dev${NC}"
    else
        echo -e "${RED}  ✗ $dev (missing!)${NC}"
        exit 1
    fi
done

# Step 3: Build if needed
if [ ! -f "$BIN" ]; then
    echo ""
    echo -e "${YELLOW}[ChessBot] Binary not found. Building...${NC}"
    make -C "$SCRIPT_DIR"
fi

# Step 4: Grant device permissions
echo -e "${CYAN}[ChessBot] Granting permissions to devices so GUI can run without root...${NC}"
sudo -S chmod 666 /dev/rkkdr_mouse /dev/rkkdr_keyboard /dev/rkkdr_screen /dev/input/event* 2>/dev/null || true

# Step 5: Launch the bot
echo ""
echo -e "${GREEN}[ChessBot] Launching ChessBot...${NC}"
echo -e "${CYAN}========================================${NC}"
echo ""

"$BIN" "$@"
