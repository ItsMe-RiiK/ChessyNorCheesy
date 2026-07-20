#!/bin/bash
# ============================================================
# launcher.sh — Master launcher for ChessyNotCheesy
# 1. Updates Stockfish to latest version
# 2. Launches the chess bot
# ============================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$SCRIPT_DIR/release/ChessyNotCheesy"

CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  ChessyNotCheesy Launcher${NC}"
echo -e "${CYAN}========================================${NC}"

if [ ! -f "$BIN" ]; then
    echo -e "${RED}[ChessyNotCheesy] Binary not found at $BIN. Please ensure you ran install.sh or downloaded a valid release.${NC}"
    # Wait for user to read message before closing if launched from terminal (if applicable)
    sleep 3
    exit 1
fi

echo -e "${GREEN}[ChessyNotCheesy] Launching...${NC}"
echo -e "${CYAN}========================================${NC}"

cd "$SCRIPT_DIR"
"$BIN" "$@"
