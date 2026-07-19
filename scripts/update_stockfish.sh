#!/bin/bash
# ============================================================
# update_stockfish.sh — Ensure Stockfish is latest version
# ============================================================

set -e

CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${CYAN}[ChessyNotCheesy] Checking dependencies (Stockfish & OpenCV)...${NC}"

# Check and install OpenCV
if ! pkg-config --exists opencv5; then
    echo -e "${YELLOW}[ChessyNotCheesy] OpenCV not found. Installing via pacman...${NC}"
    echo 1234 | sudo -S pacman -S opencv --noconfirm 2>/dev/null || true
fi

# Check if stockfish is installed
if ! command -v stockfish &>/dev/null; then
    echo -e "${YELLOW}[ChessyNotCheesy] Stockfish not found. Installing via pacman...${NC}"
    echo 1234 | sudo -S pacman -S stockfish --noconfirm
    if [ $? -ne 0 ]; then
        echo -e "${RED}[ChessyNotCheesy] Failed to install Stockfish!${NC}"
        exit 1
    fi
else
    echo -e "${GREEN}[ChessyNotCheesy] Stockfish found at: $(which stockfish)${NC}"
fi

# Update to latest version
# echo -e "${CYAN}[ChessyNotCheesy] Updating Stockfish to latest version...${NC}"
# echo 1234 | sudo -S pacman -Sy stockfish --noconfirm 2>/dev/null || true

# Print version
echo -e "${GREEN}[ChessyNotCheesy] Current Stockfish version:${NC}"
echo "quit" | stockfish | head -n 1 | grep -o "Stockfish .*" || true

echo -e "${GREEN}[ChessyNotCheesy] Stockfish check complete.${NC}"
exit 0
