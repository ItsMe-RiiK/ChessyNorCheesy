#!/bin/bash
# ============================================================
# update_app.sh — Update ChessyNotCheesy to latest version
# ============================================================

set -e

CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$SCRIPT_DIR"

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  ChessyNotCheesy Updater${NC}"
echo -e "${CYAN}========================================${NC}"

echo -e "${GREEN}[Update] Pulling latest changes from GitHub...${NC}"
git pull origin main || git pull origin master || true

echo -e "${GREEN}[Update] Rebuilding project...${NC}"
make clean
make

echo -e "${GREEN}[Update] Checking dependencies...${NC}"
bash "$SCRIPT_DIR/scripts/update_stockfish.sh"

echo -e "${GREEN}[Update] Update complete!${NC}"
exit 0
