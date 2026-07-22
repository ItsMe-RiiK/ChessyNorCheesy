#!/bin/bash

# setup_stockfish.sh
# Downloads and extracts the official Stockfish binary for Linux

set -e

# Configuration
STOCKFISH_VERSION="18"
STOCKFISH_TAR="stockfish-ubuntu-x86-64-avx2.tar"
DOWNLOAD_URL="https://github.com/official-stockfish/Stockfish/releases/download/sf_${STOCKFISH_VERSION}/${STOCKFISH_TAR}"
BIN_DIR="bin"
EXECUTABLE_NAME="stockfish"

# Go to project root
cd "$(dirname "$0")/.."

echo "Setting up Stockfish ${STOCKFISH_VERSION}..."

mkdir -p "$BIN_DIR"
cd "$BIN_DIR"

echo "Downloading from: ${DOWNLOAD_URL}"
curl -L -o "${STOCKFISH_TAR}" "${DOWNLOAD_URL}"

echo "Extracting..."
mkdir -p sf_temp
tar -xf "${STOCKFISH_TAR}" -C sf_temp

# Find the executable inside the temp dir and copy it to bin/stockfish
find sf_temp -type f -name "stockfish*" -executable | head -n 1 | xargs -I {} cp {} "$EXECUTABLE_NAME"

# Cleanup
rm -rf "${STOCKFISH_TAR}" sf_temp

echo "Stockfish setup complete! Binary is located at ${BIN_DIR}/${EXECUTABLE_NAME}"
echo "You can test it by running: ./${BIN_DIR}/${EXECUTABLE_NAME}"
