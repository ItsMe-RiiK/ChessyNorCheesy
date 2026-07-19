# ChessyNotCheesy

<p align="center">
    <img src="images/Icon_256.png" alt="Logo" width="175"/>
</p>

**ChessyNotCheesy** is a highly optimized, fully autonomous computer-vision based chess bot designed to play chess on Chess.com directly from your Linux desktop, using the powerful Stockfish engine.

Unlike traditional chess bots or extensions that inject JavaScript, read browser memory, or hook into the DOM, **ChessyNotCheesy operates entirely outside the browser**. It works exactly like a human player: it "looks" at your screen using X11 screen capture, and it "clicks" the mouse using hardware-level input simulation (`XTest`).

---

## ✨ Features

- **100% External Vision System**: Uses OpenCV `TM_SQDIFF_NORMED` template matching to read the board visually. It is completely immune to board highlights, last-move indicators, and square colors.
- **Human-like Interaction**: Uses `XTestFakeMotionEvent` and `XTestFakeButtonEvent` to simulate real mouse movements and clicks.
- **Built-in GTK3 GUI**:
  - Real-time PGN generation and tracking.
  - Live engine evaluation (e.g. `eval: +1.20`).
  - Adjustable Stockfish calculation depth (defaults to 5).
  - Configurable artificial delay between moves to mimic human thinking time.
  - Global Hotkeys: `` ` `` to start/stop, `c` to calibrate, `r` to instantly reset game memory, `1` for White, `2` for Black.
- **Robust State Engine**: Handles move parsing and en-passant/castling rules without relying on algebraic notation parsing from the website.

## ️ Requirements & Dependencies

This project is built exclusively for Linux (specifically tested on Arch Linux / X11). It will not work out-of-the-box on Wayland due to its reliance on X11 and XTest APIs.

- `g++` (Compiler with C++17 support)
- `make`
- `stockfish` (Required for move calculation)
- `opencv` (libopencv-dev, specifically OpenCV 5 or compatible)
- `gtk3`
- `x11` and `xtst` (libxtst-dev)

### Installation

An automated script is provided to install all necessary dependencies for Arch, Debian/Ubuntu, and Fedora systems:
```bash
./scripts/install_dependencies.sh
```
*(You may be prompted for your sudo password to install the packages)*

## 🚀 Building & Running

1. Clone the repository:
```bash
git clone https://github.com/ItsMe-RiiK/ChessyNotCheesy.git
cd ChessyNotCheesy
```

2. Run the included launcher script. It will automatically compile the project on the first run and start the bot:
```bash
./launcher.sh
```
*Note: The launcher may prompt for your `sudo` password to run `chmod 666 /dev/input/event*` so it can listen to global keyboard hotkeys while minimized.*

## 🎮 How to Use

1. **Setup Chess.com**: 
   - Open a game on Chess.com.
   - **Important:** Set your piece style to **Neo** and board style to **Wood** (or update the templates in `themes/pieces/neo` to match your preferred theme).
   - Set move method: drag or click
   - Set piece animation: slow or medium (Default)
   - Make sure the entire board is visible on your primary monitor without obstruction.

2. **Calibrate**:
   - In the GUI, click `(C)alibrate` or press the `c` hotkey.
   - Click exactly on the **Top-Left corner** of the board (the top-left edge of the `a8` square).
   - Click exactly on the **Bottom-Right corner** of the board (the bottom-right edge of the `h1` square).
   - The bot will calculate the square sizes and lock its vision onto the board.

3. **Play**:
   - Select your color side (`1` for White, `2` for Black).
   - Press the backtick `` ` `` key to start the bot.
   - Watch it play automatically!
   - If the game ends, press `r` to reset the bot's memory for the next game.

## ⚠️ Disclaimer

This project is strictly for **educational and research purposes**. Using automated bots on Chess.com violates their Terms of Service and will likely result in your account being banned. i do not have responsibility for any consequences that arise from using this software. Do not use this tool to cheat against human players.


## License

This project is licensed under the GPL-3.0 License - see the [LICENSE](License) file for details.