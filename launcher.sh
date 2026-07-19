#!/bin/bash
# ============================================================
# launcher.sh — Master launcher for ChessBot
# 1. Updates Stockfish to latest version
# 2. Launches the chess bot
# ============================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$SCRIPT_DIR/release/ChessBot"

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

# Step 3: Build if needed
if [ ! -f "$BIN" ]; then
    echo ""
    echo -e "${YELLOW}[ChessBot] Binary not found. Building...${NC}"
    make -C "$SCRIPT_DIR"
fi

# Step 4: Grant permissions to input events for X11/Mouse if needed
echo -e "${CYAN}[ChessBot] Checking event permissions...${NC}"
echo "1234" | sudo -S chmod 666 /dev/input/event* 2>/dev/null || true

# Step 5: Launch the bot
echo ""
echo -e "${GREEN}[ChessBot] Launching ChessBot...${NC}"
echo -e "${CYAN}========================================${NC}"
echo ""

"$BIN" "$@"
