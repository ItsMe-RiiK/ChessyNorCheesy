#!/bin/bash
# ============================================================
# uninstall.sh — Remove ChessyNotCheesy from Desktop
# ============================================================

set -e

CYAN='\033[0;36m'
GREEN='\033[0;32m'
NC='\033[0m'

DESKTOP_DIR="$HOME/Desktop"
DESKTOP_FILE="$DESKTOP_DIR/ChessyNotCheesy.desktop"
ALT_DESKTOP_FILE="$HOME/.local/share/applications/ChessyNotCheesy.desktop"

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  ChessyNotCheesy Uninstaller${NC}"
echo -e "${CYAN}========================================${NC}"

if [ -f "$DESKTOP_FILE" ]; then
    rm "$DESKTOP_FILE"
    echo -e "${GREEN}[Uninstall] Removed desktop shortcut from $DESKTOP_FILE.${NC}"
fi

if [ -f "$ALT_DESKTOP_FILE" ]; then
    rm "$ALT_DESKTOP_FILE"
    echo -e "${GREEN}[Uninstall] Removed desktop shortcut from $ALT_DESKTOP_FILE.${NC}"
fi

echo -e "${GREEN}[Uninstall] Uninstallation complete.${NC}"
exit 0
