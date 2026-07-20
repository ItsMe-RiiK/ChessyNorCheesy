#!/bin/bash
# ============================================================
# install.sh — Install ChessyNotCheesy to Desktop
# ============================================================

set -e

CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DESKTOP_DIR="$HOME/Desktop"
DESKTOP_FILE="$DESKTOP_DIR/ChessyNotCheesy.desktop"

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  ChessyNotCheesy Installer${NC}"
echo -e "${CYAN}========================================${NC}"

if [ ! -d "$DESKTOP_DIR" ]; then
    echo -e "${YELLOW}[Install] Desktop directory not found at $DESKTOP_DIR. Using standard applications folder...${NC}"
    DESKTOP_DIR="$HOME/.local/share/applications"
    DESKTOP_FILE="$DESKTOP_DIR/ChessyNotCheesy.desktop"
    mkdir -p "$DESKTOP_DIR"
fi

echo -e "${GREEN}[Install] Creating desktop shortcut at $DESKTOP_FILE...${NC}"

cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=ChessyNotCheesy
Comment=Autonomous computer-vision chess bot
Exec=bash "$SCRIPT_DIR/launcher.sh"
Icon=$SCRIPT_DIR/images/Icon_256.png
Terminal=true
Categories=Game;BoardGame;
EOF

chmod +x "$DESKTOP_FILE"
gio set "$DESKTOP_FILE" metadata::trusted true 2>/dev/null || true

echo -e "${GREEN}[Install] Done! You can now launch ChessyNotCheesy from your Desktop.${NC}"
exit 0
