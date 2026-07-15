#ifndef CHESSBOT_BOARD_READER_H
#define CHESSBOT_BOARD_READER_H

#include "../capture/theme_manager.h"
#include "../capture/screen_capture.h"
#include <opencv2/opencv.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <string>

/*
 * BoardReader — Reads the chess board state from screen pixels
 *
 * Uses OpenCV template matching to detect pieces on each square
 * of the chess.com board. Supports auto-calibration using the
 * board corner template.
 */

// Chess piece types
enum class Piece : int
{
  EMPTY = 0,
  WHITE_PAWN,
  WHITE_KNIGHT,
  WHITE_BISHOP,
  WHITE_ROOK,
  WHITE_QUEEN,
  WHITE_KING,
  BLACK_PAWN,
  BLACK_KNIGHT,
  BLACK_BISHOP,
  BLACK_ROOK,
  BLACK_QUEEN,
  BLACK_KING
};

// 8x8 board representation
using Board = std::array<std::array<Piece, 8>, 8>;

class BoardReader
{
public:
  BoardReader(ScreenCapture &capture, ThemeManager &theme_manager);
  ~BoardReader() = default;

  // Manual calibration (fallback)
  void calibrate(int top_left_x, int top_left_y,
                 int bottom_right_x, int bottom_right_y);

  // Auto calibration using template matching on screen
  // Returns true and populates detected_board and detected_pieces if successful
  bool auto_calibrate(std::string &detected_board, std::string &detected_pieces);

  // Set whether playing as white (true) or black (false)
  void set_playing_white(bool white);
  bool is_playing_white() const;

  // Is calibrated?
  bool is_calibrated() const;

  // Read the current board state from the screen using templates
  Board read_board();

  // Get the screen coordinates for the center of a specific square
  void get_square_center(int file, int rank, int &x, int &y) const;
  int get_square_size() const;

  // Convert piece to FEN character
  static char piece_to_fen(Piece p);

  // Convert board to printable string (for debugging)
  static std::string board_to_string(const Board &board);

  // Calibration callback — set this to receive calibration click coordinates
  std::function<void(int, int)> on_calibration_click;

private:
  ScreenCapture &capture_;
  ThemeManager &theme_manager_;
  bool is_calibrated_;
  bool playing_white_;

  // Board dimensions
  int board_tl_x_;
  int board_tl_y_;
  int board_br_x_;
  int board_br_y_;
  int square_size_;

  // OpenCV helper for converting ScreenCapture BGRA buffer to cv::Mat
  cv::Mat capture_to_mat();
};

#endif /* CHESSBOT_BOARD_READER_H */
