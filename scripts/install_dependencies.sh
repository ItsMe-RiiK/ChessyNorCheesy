#!/bin/bash
# ============================================================
# install_dependencies.sh — Install required packages
# ============================================================

set -e

echo "Detecting package manager..."

if command -v pacman &> /dev/null; then
    echo "Arch Linux detected. Installing dependencies via pacman..."
    sudo pacman -S --needed base-devel stockfish opencv gtk3 libxtst
elif command -v apt-get &> /dev/null; then
    echo "Debian/Ubuntu detected. Installing dependencies via apt..."
    sudo apt-get update
    sudo apt-get install -y build-essential stockfish libopencv-dev libgtk-3-dev libxtst-dev
elif command -v dnf &> /dev/null; then
    echo "Fedora detected. Installing dependencies via dnf..."
    sudo dnf install -y gcc-c++ make stockfish opencv-devel gtk3-devel libXtst-devel
else
    echo "Unsupported package manager. Please install dependencies manually:"
    echo "Required: g++, make, stockfish, opencv (v5/v4), gtk3, libxtst"
    exit 1
fi

echo "All dependencies installed successfully!"
