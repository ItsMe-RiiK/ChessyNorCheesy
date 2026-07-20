# ChessyNotCheesy

<p align="center">
    <img src="images/Icon_256.png" alt="Logo" width="125"/>
</p>

**ChessyNotCheesy** is a highly optimized, fully autonomous computer-vision based chess bot designed to play on Chess.com directly from your Linux desktop, powered by the **Stockfish** engine.

Unlike traditional chess bots or browser extensions that inject JavaScript, read browser memory, or hook into the DOM, **ChessyNotCheesy operates entirely outside the browser**. It acts exactly like a human player: it "looks" at your screen using X11 screen capture and "clicks" the mouse using hardware-level input simulation (`XTest`).

---

## 📸 Preview
<summary>
<div align="center">
  <b>Calibration & Setup</b><br>
  <img src="images/Setup.gif" alt="Setup" width="600"/><br><br>

  <b>In-Match Gameplay</b><br>
  <img src="images/Play.gif" alt="Play" width="600"/>
</div>

## ✨ Features

- **100% External Vision System**: Uses OpenCV `TM_SQDIFF_NORMED` template matching to read the board visually. Completely immune to board highlights, last-move indicators, and square colors.
- **CPU Efficient**: Built on lightweight `X11` screen capture and native C++ processing.
- **Human-like Interaction**: Uses `XTestFakeMotionEvent` and `XTestFakeButtonEvent` to simulate realistic mouse movements and hardware clicks.
- **Built-in GTK3 GUI**:
  - Real-time PGN generation and tracking.
  - Live engine evaluation (e.g., `eval: +1.20`).
  - Adjustable Stockfish calculation depth (defaults to 4).
  - Configurable artificial delay between moves to mimic human thinking time.
  - Global Hotkeys: `` ` `` to start/stop, `C` to calibrate, `R` to reset game memory, `1` for White, `2` for Black, `LEFTSHIFT + -/+` to adjust min mouse delay, `RIGHTSHIFT + -/+` to adjust max mouse delay.
- **Robust State Engine**: Handles move parsing, en-passant, and castling rules seamlessly without parsing algebraic notation from the website DOM.

---

## 🛠️ Requirements & Dependencies
<details>
<summary><b>Requirements</b></summary>

### 🖥️ Requirements
This project is built exclusively for Linux and is specifically tested on **Arch Linux / X11**. 
*(Note: It will not work out-of-the-box on Wayland due to its reliance on the X11 and XTest APIs).*
</details>

<details>
<summary><b>Dependencies</b></summary>

### ⛓️ Dependencies
To compile and run this project, the following packages must be installed:
- `g++` (Compiler with C++17 support)
- `make`
- `stockfish` (Required for move calculation)
- `opencv` (libopencv-dev, specifically OpenCV 5 or compatible)
- `gtk3`
- `x11` and `xtst` (libxtst-dev)

**Automated Installation:**
An automated script is provided to install all necessary dependencies for Arch, Debian/Ubuntu, and Fedora systems:
```bash
./scripts/install_dependencies.sh
```
*(You may be prompted for your sudo password to install the packages).*
</details>

---
## 🚀 Getting Started
<details>
<summary><b>Option 1: Pre-compiled Releases (Recommended)</b></summary>

Whenever a new version is released, an automated GitHub Action compiles it and packages it. You can download the latest `.tar.gz` archive directly from the [Releases page](https://github.com/ItsMe-RiiK/ChessyNotCheesy/releases).

1. Download `ChessyNotCheesy-linux-x86_64.tar.gz`.
2. Extract the archive.
3. Run the installer script to set up dependencies, udev permissions (so the bot doesn't need `sudo`), and create a Desktop shortcut:
```bash
cd ChessyNotCheesy-Release
./install.sh
```
4. You can now launch **ChessyNotCheesy** purely from your Desktop application menu like a standard GUI app!
</details>

<details>
<summary><b>Option 2: Build from Source</b></summary>

1. Clone the repository:
```bash
git clone https://github.com/ItsMe-RiiK/ChessyNotCheesy.git
cd ChessyNotCheesy
```
2. **(Optional)** For automated dependency checks without manual password prompts, create a local `.env` file:
```bash
cp .env.example .env
```
*Edit `.env` and configure your `SUDO_PASS`. This file is ignored by Git and remains strictly local.*

3. Run the installer to setup dependencies and udev permissions:
```bash
./scripts/install.sh
```
4. Compile the project:
```bash
make -j$(nproc)
```
5. Launch the bot:
```bash
./launcher.sh
```
</details>

---
## 🖥️ Application Scripts
We provide convenient scripts to manage your installation (bundled in the root folder of releases, or in the `scripts/` folder if building from source):

- **Install/Setup (`install.sh`)**: Installs Arch Linux dependencies, configures `udev` rules so the bot can listen to global hotkeys without requiring `sudo` on every launch, and creates a pure GUI desktop shortcut.
- **Update (`update.sh`)**: Automatically fetches the latest `.tar.gz` release from GitHub, downloads it, extracts it, and overwrites your current installation seamlessly.
- **Uninstall (`uninstall.sh`)**: Removes the desktop shortcuts and cleans up the `udev` rules from your system.

---
## 🎮 How to Use

1. **Setup Chess.com**: 
   - Open a game on Chess.com.
   - **Important:** Set your piece style to **Neo** and board style to **Wood**. If you prefer other styles, you must update the templates in `themes/pieces/neo` and adjust the [Default Config](themes/default.cfg).
   - Set move method to **Drag or Click**.
   - Set piece animation to **Slow** or **Medium** (Default).
   - Ensure the entire board is visible on your primary monitor without obstruction.

2. **Calibrate**:
   - In the GUI, click `(C)alibrate` or press the `C` hotkey.
   - Click exactly on the **Top-Left corner** of the board (the top-left edge of the `a8` square).
   - Click exactly on the **Bottom-Right corner** of the board (the bottom-right edge of the `h1` square).
   - The bot will automatically calculate square sizes and lock its vision onto the board.

3. **Play**:
   - Select your color side (`1` for White, `2` for Black).
   - Press the backtick `` ` `` key to start the bot.
   - Watch it play automatically!
   - When the game ends, press `R` to reset the bot's memory for the next game.

---

## ⚠️ Disclaimer

This project is strictly for **educational and research purposes**. Using automated bots on Chess.com violates their Terms of Service and will likely result in your account being banned. The developers hold no responsibility for any consequences that arise from using this software. **Do not use this tool to cheat against human players.**

---

## 📝 License

This project is licensed under the GPL-3.0 License - see the [LICENSE](License) file for details.