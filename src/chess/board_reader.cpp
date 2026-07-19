#include "board_reader.h"

#include <algorithm>
#include <cstdio>
#include <iostream>

BoardReader::BoardReader(ScreenCapture &capture, ThemeManager &theme_manager) :
    capture_(capture), theme_manager_(theme_manager), is_calibrated_(false), playing_white_(true), board_tl_x_(0), board_tl_y_(0),
    board_br_x_(0), board_br_y_(0), square_size_(0)
{
}

cv::Mat BoardReader::capture_to_mat()
{
  const uint8_t *buffer = capture_.get_buffer();
  if (!buffer)
    return cv::Mat();

  // ScreenCapture returns BGRA format pixels
  cv::Mat img(capture_.get_capture_height(), capture_.get_capture_width(), CV_8UC4, (void *)buffer, capture_.get_bytes_per_line());

  // OpenCV matchTemplate often works better with BGR or grayscale
  cv::Mat bgr;
  cv::cvtColor(img, bgr, cv::COLOR_BGRA2BGR, 3);
  return bgr;
}

void BoardReader::calibrate(int top_left_x, int top_left_y, int bottom_right_x, int bottom_right_y)
{
  board_tl_x_ = std::min(top_left_x, bottom_right_x);
  board_tl_y_ = std::min(top_left_y, bottom_right_y);
  board_br_x_ = std::max(top_left_x, bottom_right_x);
  board_br_y_ = std::max(top_left_y, bottom_right_y);

  int board_w = board_br_x_ - board_tl_x_;
  int sq_size = board_w / 8;

  if (sq_size < 10)
  {
    std::cerr << "[ChessBot][BoardReader] Invalid calibration: board too small.\n";
    return;
  }

  square_size_ = sq_size;
  is_calibrated_ = true;

  printf(
      "[ChessBot][BoardReader] Calibrated: board at (%d, %d), "
      "square size %d\n",
      board_tl_x_, board_tl_y_, square_size_
  );
}

void BoardReader::set_playing_white(bool white)
{
  playing_white_ = white;
}

bool BoardReader::is_playing_white() const
{
  return playing_white_;
}

bool BoardReader::is_calibrated() const
{
  return is_calibrated_;
}

int BoardReader::get_square_size() const
{
  return square_size_;
}

void BoardReader::get_square_center(int file, int rank, int &screen_x, int &screen_y) const
{
  // file: 0=a, 7=h — rank: 0=1, 7=8
  // If playing white: a8 is at top-left
  int screen_file, screen_rank;

  if (playing_white_)
  {
    screen_file = file;
    screen_rank = 7 - rank;
  }
  else
  {
    screen_file = 7 - file;
    screen_rank = rank;
  }

  screen_x = board_tl_x_ + (screen_file * square_size_) + (square_size_ / 2);
  screen_y = board_tl_y_ + (screen_rank * square_size_) + (square_size_ / 2);
}

void BoardReader::clear_empty_templates()
{
  empty_light_templ_.release();
  empty_dark_templ_.release();
}

void BoardReader::extract_empty_templates(const cv::Mat &screen)
{
  if (!empty_light_templ_.empty() && !empty_dark_templ_.empty())
    return;

  int best_light_edges = 1e9;
  int best_dark_edges = 1e9;

  // Find the light and dark squares with the absolute minimum number of Canny edges.
  // Pieces have strong outlines (high edge count). Empty squares (even with wood grain) have very few edges.
  for (int rank = 0; rank < 8; rank++)
  {
    for (int file = 0; file < 8; file++)
    {
      int screen_file = playing_white_ ? file : (7 - file);
      int screen_rank = playing_white_ ? (7 - rank) : rank;

      // Add the 16 pixel margin offset since the screen Mat was captured with margin
      int margin = 16;
      int sx = (screen_file * square_size_) + margin;
      int sy = (screen_rank * square_size_) + margin;

      cv::Rect square_roi(sx, sy, square_size_, square_size_);
      if (square_roi.x < 0 || square_roi.y < 0 || square_roi.x + square_roi.width > screen.cols ||
          square_roi.y + square_roi.height > screen.rows)
        continue;

      cv::Mat square_img = screen(square_roi);

      // Crop the center 50% to avoid coordinates
      int margin_x = square_img.cols * 0.25;
      int margin_y = square_img.rows * 0.25;
      cv::Rect center_roi(margin_x, margin_y, square_img.cols - 2 * margin_x, square_img.rows - 2 * margin_y);
      cv::Mat center_img = square_img(center_roi);

      cv::Mat gray, edges;
      cv::cvtColor(center_img, gray, cv::COLOR_BGR2GRAY);
      cv::Canny(gray, edges, 50, 150);
      int edge_count = cv::countNonZero(edges);

      bool is_dark = ((rank + file) % 2 == 0);

      // We take the square with the least amount of edges (most likely to be completely empty)
      if (is_dark)
      {
        if (edge_count < best_dark_edges)
        {
          best_dark_edges = edge_count;
          empty_dark_templ_ = square_img.clone();
        }
      }
      else
      {
        if (edge_count < best_light_edges)
        {
          best_light_edges = edge_count;
          empty_light_templ_ = square_img.clone();
        }
      }
    }
  }

  if (!empty_light_templ_.empty() && !empty_dark_templ_.empty())
  {
    printf("[ChessBot][BoardReader] Extracted dynamic empty templates (edges: L=%d, D=%d)\n", best_light_edges, best_dark_edges);
  }
}

Board BoardReader::read_board()
{
  Board board = {};
  if (!is_calibrated_)
    return board;

  // Increase margin to 16 to allow for up to 16 pixels of manual calibration error or window shift
  int margin = 16;
  if (!capture_.capture_region(
          board_tl_x_ - margin, board_tl_y_ - margin, (square_size_ * 8) + (margin * 2), (square_size_ * 8) + (margin * 2)
      ))
    return board;

  cv::Mat screen = capture_to_mat();
  if (screen.empty())
    return board;

  // Extract empty templates on the fly from the current screen
  extract_empty_templates(screen);

  // For every square, we crop it and match against all piece templates
  for (int rank = 0; rank < 8; rank++)
  {
    for (int file = 0; file < 8; file++)
    {
      int screen_file = playing_white_ ? file : (7 - file);
      int screen_rank = playing_white_ ? (7 - rank) : rank;

      // Add the margin offset to the local coordinates since the captured screen includes the margin
      int sx = (screen_file * square_size_) + margin;
      int sy = (screen_rank * square_size_) + margin;

      // Define square ROI with a margin to tolerate slight calibration errors or window drift
      int roi_x = std::max(0, sx - margin);
      int roi_y = std::max(0, sy - margin);
      int roi_w = square_size_ + (margin * 2);
      int roi_h = square_size_ + (margin * 2);

      // Clamp width and height to screen bounds
      if (roi_x + roi_w > screen.cols)
        roi_w = screen.cols - roi_x;
      if (roi_y + roi_h > screen.rows)
        roi_h = screen.rows - roi_y;

      cv::Rect square_roi(roi_x, roi_y, roi_w, roi_h);
      if (square_roi.width <= 0 || square_roi.height <= 0)
      {
        continue;
      }

      cv::Mat square_img = screen(square_roi);

      // Fast-path: An empty square (even highlighted yellow) has no edges in its center.
      // We crop the center to avoid rank/file coordinates printed on the edges.
      cv::Mat square_gray;
      cv::cvtColor(square_img, square_gray, cv::COLOR_BGR2GRAY);
      int c_margin_x = square_gray.cols * 0.2;
      int c_margin_y = square_gray.rows * 0.2;
      cv::Rect center_roi(c_margin_x, c_margin_y, square_gray.cols - 2 * c_margin_x, square_gray.rows - 2 * c_margin_y);
      cv::Mat center_gray = square_gray(center_roi);

      cv::Mat edges;
      cv::Canny(center_gray, edges, 50, 150);
      if (cv::countNonZero(edges) < 15)
      {
        board[rank][file] = Piece::EMPTY;
        continue;
      }

      double best_match = 1e9; // SQDIFF is lower-is-better
      Piece best_piece = Piece::EMPTY;

      // Apply blur to square
      cv::Mat square_blur;
      cv::GaussianBlur(square_img, square_blur, cv::Size(3, 3), 0);

      // Loop through pieces 1 to 12
      for (int p = 1; p <= 12; p++)
      {
        cv::Mat templ = theme_manager_.get_template(p);
        cv::Mat mask = theme_manager_.get_mask(p);
        if (templ.empty())
          continue;

        cv::Mat search_templ = templ;
        cv::Mat search_mask = mask;
        if (search_templ.cols != square_size_ || search_templ.rows != square_size_)
        {
          cv::resize(search_templ, search_templ, cv::Size(square_size_, square_size_), 0, 0, cv::INTER_LINEAR);
          if (!search_mask.empty())
          {
            cv::threshold(search_mask, search_mask, 240, 255, cv::THRESH_BINARY);
            cv::resize(search_mask, search_mask, cv::Size(square_size_, square_size_), 0, 0, cv::INTER_NEAREST);
            cv::erode(search_mask, search_mask, cv::Mat(), cv::Point(-1, -1), 1);
          }
        }

        cv::Mat templ_blur;
        cv::GaussianBlur(search_templ, templ_blur, cv::Size(3, 3), 0);

        cv::Mat result;
        if (!search_mask.empty())
        {
          cv::Mat search_mask_3c;
          cv::cvtColor(search_mask, search_mask_3c, cv::COLOR_GRAY2BGR);
          cv::matchTemplate(square_blur, templ_blur, result, cv::TM_SQDIFF_NORMED, search_mask_3c);
        }
        else
        {
          cv::matchTemplate(square_blur, templ_blur, result, cv::TM_SQDIFF_NORMED);
        }

        double minVal, maxVal;
        cv::minMaxLoc(result, &minVal, &maxVal);

        if (minVal < best_match)
        {
          best_match = minVal;
          best_piece = static_cast<Piece>(p);
        }
      }

      // Now compare against the EMPTY template for this square's color
      bool is_dark = ((rank + file) % 2 == 0);
      cv::Mat empty_templ = is_dark ? empty_dark_templ_ : empty_light_templ_;

      if (!empty_templ.empty())
      {
        cv::Mat search_empty = empty_templ;
        if (search_empty.cols != square_size_ || search_empty.rows != square_size_)
        {
          cv::resize(search_empty, search_empty, cv::Size(square_size_, square_size_), 0, 0, cv::INTER_LINEAR);
        }

        cv::Mat empty_blur;
        cv::GaussianBlur(search_empty, empty_blur, cv::Size(3, 3), 0);

        cv::Mat result_empty;
        cv::matchTemplate(square_blur, empty_blur, result_empty, cv::TM_SQDIFF_NORMED);

        double emptyMin, emptyMax;
        cv::minMaxLoc(result_empty, &emptyMin, &emptyMax);

        // If the square looks more like an empty square than any piece, it's empty!
        if (emptyMin < best_match)
        {
          best_match = emptyMin;
          best_piece = Piece::EMPTY;
        }
      }

      board[rank][file] = best_piece;
    }
  }

  return board;
}

char BoardReader::piece_to_fen(Piece p)
{
  switch (p)
  {
  case Piece::WHITE_PAWN:
    return 'P';
  case Piece::WHITE_KNIGHT:
    return 'N';
  case Piece::WHITE_BISHOP:
    return 'B';
  case Piece::WHITE_ROOK:
    return 'R';
  case Piece::WHITE_QUEEN:
    return 'Q';
  case Piece::WHITE_KING:
    return 'K';
  case Piece::BLACK_PAWN:
    return 'p';
  case Piece::BLACK_KNIGHT:
    return 'n';
  case Piece::BLACK_BISHOP:
    return 'b';
  case Piece::BLACK_ROOK:
    return 'r';
  case Piece::BLACK_QUEEN:
    return 'q';
  case Piece::BLACK_KING:
    return 'k';
  default:
    return '.';
  }
}

std::string BoardReader::board_to_string(const Board &board)
{
  std::string s;
  for (int rank = 7; rank >= 0; rank--)
  {
    for (int file = 0; file < 8; file++)
    {
      s += piece_to_fen(board[rank][file]);
      s += ' ';
    }
    s += '\n';
  }
  return s;
}
