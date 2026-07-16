#include "bot_controller.h"

#include <chrono>
#include <cstdio>
#include <random>

static std::mt19937 &get_bot_rng()
{
  static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
  return rng;
}

BotController::BotController() :
    board_reader_(capture_, theme_manager_), running_(false), should_stop_(false), stockfish_depth_(20), move_delay_min_ms_(200),
    move_delay_max_ms_(800), poll_interval_ms_(200)
{
}

BotController::~BotController()
{
  stop();
  cleanup();
}

bool BotController::init()
{
  set_status("Initializing...");

  // Initialize screen capture (X11)
  if (!capture_.init())
  {
    set_status("ERROR: Failed to init screen capture (X11)");
    return false;
  }

  // Initialize mouse driver
  if (!mouse_.open())
  {
    set_status("ERROR: Failed to open /dev/rkkdr_mouse");
    return false;
  }

  // Initialize Stockfish
  if (!stockfish_.start())
  {
    set_status("ERROR: Failed to start Stockfish");
    return false;
  }

  // Configure Stockfish for performance
  stockfish_.set_threads(4);
  stockfish_.set_hash(256);

  // Initialize HTTP Server
  http_server_.start(8080);

  set_status("Ready. Auto-Detect or Calibrate to begin.");
  printf("[ChessBot] All systems initialized.\n");

  return true;
}

void BotController::cleanup()
{
  stop();
  http_server_.stop();
  stockfish_.stop();
  mouse_.close();
  capture_.cleanup();
}

void BotController::start()
{
  if (running_)
    return;

  if (!board_reader_.is_calibrated())
  {
    set_status("ERROR: Board not calibrated. Click two corners first.");
    return;
  }

  should_stop_ = false;
  running_ = true;

  bot_thread_ = std::thread(&BotController::bot_loop, this);

  set_status("Bot RUNNING — watching for moves...");
}

void BotController::stop()
{
  if (!running_)
    return;

  should_stop_ = true;

  if (bot_thread_.joinable())
    bot_thread_.join();

  running_ = false;
  set_status("Bot STOPPED.");
}

bool BotController::is_running() const
{
  return running_;
}

void BotController::calibrate(int tl_x, int tl_y, int br_x, int br_y)
{
  board_reader_.calibrate(tl_x, tl_y, br_x, br_y);
  game_state_.reset();

  std::string b, p;
  if (!theme_manager_.load_default_config(b, p))
  {
    auto pieces = theme_manager_.get_available_pieces();
    for (const auto &candidate : pieces)
    {
      if (theme_manager_.load_piece_theme(candidate))
      {
        p = candidate;
        break;
      }
    }
  }
  else
  {
    theme_manager_.load_piece_theme(p);
  }

  set_status("Board calibrated manually! Ready to start.");
}

bool BotController::auto_calibrate()
{
  std::string board_theme, piece_theme;
  if (board_reader_.auto_calibrate(board_theme, piece_theme))
  {
    game_state_.reset();
    std::string msg = "Board Auto-Detected! [Board: " + board_theme + " | Pieces: " + piece_theme + "]";
    set_status(msg);
    return true;
  }

  set_status("ERROR: Auto-Detect failed. Ensure board is visible and themes exist.");
  return false;
}

bool BotController::is_calibrated() const
{
  return board_reader_.is_calibrated();
}

void BotController::set_playing_white(bool white)
{
  board_reader_.set_playing_white(white);
  game_state_.set_playing_white(white);
}

void BotController::set_stockfish_depth(int depth)
{
  stockfish_depth_ = depth;
}

void BotController::set_move_delay(int min_ms, int max_ms)
{
  move_delay_min_ms_ = min_ms;
  move_delay_max_ms_ = max_ms;
}

void BotController::set_poll_interval_ms(int ms)
{
  poll_interval_ms_ = ms;
}

std::string BotController::get_current_fen() const
{
  return game_state_.to_fen();
}

std::string BotController::get_last_move() const
{
  return game_state_.get_last_move();
}

std::string BotController::get_engine_eval() const
{
  return engine_eval_;
}

std::string BotController::get_status() const
{
  return current_status_;
}

int BotController::get_stockfish_depth() const
{
  return stockfish_depth_;
}

void BotController::set_status(const std::string &status)
{
  current_status_ = status;
  printf("[ChessBot] %s\n", status.c_str());

  if (on_status_change)
    on_status_change(status);
}

void BotController::human_delay()
{
  std::uniform_int_distribution<int> dist(move_delay_min_ms_, move_delay_max_ms_);
  int ms = dist(get_bot_rng());
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void BotController::bot_loop()
{
  printf("[ChessBot] Bot loop started.\n");

  while (!should_stop_)
  {
    // Wait for FEN updates from HTTP server
    if (http_server_.has_new_fen())
    {
      std::string fen = http_server_.pop_fen();
      printf("[ChessBot][HTTP] Received FEN: %s\n", fen.c_str());

      // Parse FEN into a Board object
      Board detected_board;
      for (auto &row : detected_board)
        row.fill(Piece::EMPTY);

      int r = 7, f = 0;
      for (char c : fen)
      {
        if (c == ' ')
          break; // Only care about piece placement
        if (c == '/')
        {
          r--;
          f = 0;
        }
        else if (isdigit(c))
        {
          f += (c - '0');
        }
        else
        {
          Piece p = Piece::EMPTY;
          switch (c)
          {
          case 'P':
            p = Piece::WHITE_PAWN;
            break;
          case 'N':
            p = Piece::WHITE_KNIGHT;
            break;
          case 'B':
            p = Piece::WHITE_BISHOP;
            break;
          case 'R':
            p = Piece::WHITE_ROOK;
            break;
          case 'Q':
            p = Piece::WHITE_QUEEN;
            break;
          case 'K':
            p = Piece::WHITE_KING;
            break;
          case 'p':
            p = Piece::BLACK_PAWN;
            break;
          case 'n':
            p = Piece::BLACK_KNIGHT;
            break;
          case 'b':
            p = Piece::BLACK_BISHOP;
            break;
          case 'r':
            p = Piece::BLACK_ROOK;
            break;
          case 'q':
            p = Piece::BLACK_QUEEN;
            break;
          case 'k':
            p = Piece::BLACK_KING;
            break;
          }
          if (r >= 0 && r < 8 && f >= 0 && f < 8)
            detected_board[r][f] = p;
          f++;
        }
      }

      bool changed = game_state_.update(detected_board);

      if (changed)
      {
        std::string new_fen = game_state_.to_fen();
        if (on_fen_update)
          on_fen_update(new_fen);
      }
    }

    // 3. If it's our turn, calculate and make a move
    if (game_state_.is_our_turn())
    {
      set_status("Our turn — thinking...");

      // Add human-like delay before calculating
      human_delay();

      // Get current FEN
      std::string fen = game_state_.to_fen();

      // Ensure FEN is valid (has both kings) before sending to Stockfish
      if (fen.find('K') == std::string::npos || fen.find('k') == std::string::npos)
      {
        set_status("ERROR: Board missing kings! FEN: " + fen);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        continue;
      }

      // Query Stockfish
      stockfish_.set_position(fen);
      std::string best_move = stockfish_.get_best_move(stockfish_depth_);

      if (best_move.empty() || best_move == "(none)")
      {
        set_status("No valid move found — game may be over.");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        continue;
      }

      // Update eval display
      int score = stockfish_.get_last_score();
      char eval_buf[64];
      if (std::abs(score) > 90000)
      {
        int mate_in = 100000 - std::abs(score);
        snprintf(eval_buf, sizeof(eval_buf), "Mate in %d", mate_in);
      }
      else
      {
        snprintf(eval_buf, sizeof(eval_buf), "%+.2f", score / 100.0);
      }
      engine_eval_ = eval_buf;

      set_status("Playing: " + best_move + " (eval: " + engine_eval_ + ")");

      // Execute the move on screen
      if (!execute_move(best_move))
      {
        set_status("ERROR: Failed to execute move " + best_move);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        continue;
      }

      // Apply the move to our game state
      game_state_.apply_move(best_move);

      if (on_move_made)
        on_move_made(best_move);

      set_status("Waiting for opponent...");
    }

    // 4. Sleep before next poll
    std::this_thread::sleep_for(std::chrono::milliseconds(poll_interval_ms_));
  }

  printf("[ChessBot] Bot loop ended.\n");
}

bool BotController::execute_move(const std::string &uci_move)
{
  if (uci_move.size() < 4)
    return false;

  // Parse source and destination squares
  int from_file = uci_move[0] - 'a';
  int from_rank = uci_move[1] - '1';
  int to_file = uci_move[2] - 'a';
  int to_rank = uci_move[3] - '1';

  // Get screen coordinates for the squares
  int from_x, from_y, to_x, to_y;
  board_reader_.get_square_center(from_file, from_rank, from_x, from_y);
  board_reader_.get_square_center(to_file, to_rank, to_x, to_y);

  printf("[ChessBot] Move %s: click (%d,%d) → (%d,%d)\n", uci_move.c_str(), from_x, from_y, to_x, to_y);

  // Drag from source square to destination square
  if (!mouse_.drag(from_x, from_y, to_x, to_y))
    return false;

  // Handle pawn promotion
  if (uci_move.size() == 5)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    click_promotion(uci_move[4]);
  }

  return true;
}

bool BotController::click_promotion(char promo_piece)
{
  /*
   * Chess.com promotion UI: when a pawn reaches the last rank,
   * a popup appears with 4 options vertically stacked on the
   * destination file. For white promoting on rank 8:
   *   Queen  (at rank 8 position)
   *   Rook   (at rank 7 position)
   *   Bishop (at rank 6 position)
   *   Knight (at rank 5 position)
   *
   * For black promoting on rank 1:
   *   Queen  (at rank 1 position)
   *   Rook   (at rank 2 position)
   *   Bishop (at rank 3 position)
   *   Knight (at rank 4 position)
   */

  // Most common: queen promotion — just click the first option (already at dest)
  // which is usually auto-selected or at the same position
  if (promo_piece == 'q')
  {
    // Queen is the first option — already clicked at destination
    return true;
  }

  // For underpromotion, we need to click the appropriate option
  // The promotion popup squares are stacked vertically from the promotion rank
  int offset = 0;
  switch (promo_piece)
  {
  case 'r':
    offset = 1;
    break;
  case 'b':
    offset = 2;
    break;
  case 'n':
    offset = 3;
    break;
  default:
    return true;
  }

  // Calculate the click position based on square size
  int sq_size = board_reader_.get_square_size();

  // Get the last move destination to find the file
  std::string last = game_state_.get_last_move();
  if (last.size() < 4)
    return false;

  int file = last[2] - 'a';
  int rank = last[3] - '1';

  int click_x, click_y;
  board_reader_.get_square_center(file, rank, click_x, click_y);

  // Offset vertically based on playing color
  if (game_state_.is_playing_white())
  {
    click_y += offset * sq_size; // Down from rank 8
  }
  else
  {
    click_y -= offset * sq_size; // Up from rank 1
  }

  return mouse_.click(click_x, click_y);
}
