#ifndef CHESSBOT_GAME_STATE_H
#define CHESSBOT_GAME_STATE_H

#include "board_reader.h"
#include <string>
#include <vector>

/*
 * GameState — Tracks the chess game state over time
 *
 * Maintains the current position, generates FEN strings,
 * detects board changes, and tracks move history.
 * Refines piece types using starting position knowledge and move tracking.
 */
class GameState
{
public:
  GameState();
  ~GameState() = default;

  // Reset to starting position
  void reset();

  // Set which color we're playing
  void set_playing_white(bool white);
  bool is_playing_white() const;

  // Update the internal board from a detected board (from BoardReader)
  // Returns true if the board has changed since last update
  bool update(const Board &detected_board);

  // Generate FEN string from current position
  std::string to_fen() const;

  // Is it our turn to move?
  bool is_our_turn() const;

  // Has the board changed since last check?
  bool has_changed() const;

  // Get the last detected move in UCI format (e.g., "e2e4")
  std::string get_last_move() const;

  // Get move history as space-separated UCI moves
  std::string get_move_history() const;

  // Get the current board
  const Board &get_board() const;

  // Get move count (full moves)
  int get_move_count() const;

  // Detect if the game appears to be over (no pieces changed for a while)
  bool is_game_over() const;

  // Apply a UCI move to the internal board (e.g., "e2e4")
  void apply_move(const std::string &uci_move);

private:
  Board board_;
  Board prev_board_;
  bool board_changed_;
  bool playing_white_;
  bool white_to_move_;

  // Move tracking
  std::vector<std::string> move_history_;
  std::string last_move_;
  int halfmove_clock_;
  int fullmove_number_;

  // Castling rights
  bool white_castle_king_;
  bool white_castle_queen_;
  bool black_castle_king_;
  bool black_castle_queen_;

  // En passant
  std::string en_passant_square_;

  // Initial board setup
  void setup_initial_board();

  // Detect what move was made by comparing old and new boards
  std::string detect_move(const Board &old_board, const Board &new_board);

  // Update castling rights based on a move
  void update_castling(const std::string &move);

  // Convert file/rank to UCI square string
  static std::string square_to_uci(int file, int rank);

  // Parse UCI square string to file/rank
  static bool uci_to_square(const std::string &uci, int &file, int &rank);
};

#endif /* CHESSBOT_GAME_STATE_H */
