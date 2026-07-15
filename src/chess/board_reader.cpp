#include "board_reader.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>

BoardReader::BoardReader(ScreenCapture &capture, ThemeManager &theme_manager)
    : capture_(capture),
      theme_manager_(theme_manager),
      is_calibrated_(false),
      playing_white_(true),
      board_tl_x_(0), board_tl_y_(0),
      board_br_x_(0), board_br_y_(0),
      square_size_(0)
{
}

cv::Mat BoardReader::capture_to_mat()
{
  const uint8_t *buffer = capture_.get_buffer();
  if (!buffer)
    return cv::Mat();

  // ScreenCapture returns BGRA format pixels
  cv::Mat img(capture_.get_capture_height(), capture_.get_capture_width(), 
              CV_8UC4, (void*)buffer, capture_.get_bytes_per_line());
  
  // OpenCV matchTemplate often works better with BGR or grayscale
  cv::Mat bgr;
  cv::cvtColor(img, bgr, cv::COLOR_BGRA2BGR, 3);
  return bgr;
}

void BoardReader::calibrate(int top_left_x, int top_left_y,
                            int bottom_right_x, int bottom_right_y)
{
  board_tl_x_ = std::min(top_left_x, bottom_right_x);
  board_tl_y_ = std::min(top_left_y, bottom_right_y);
  board_br_x_ = std::max(top_left_x, bottom_right_x);
  board_br_y_ = std::max(top_left_y, bottom_right_y);

  int board_w = board_br_x_ - board_tl_x_;
  square_size_ = board_w / 8;

  is_calibrated_ = true;

  printf("[ChessBot][BoardReader] Calibrated: board at (%d, %d), "
         "square size %d\n",
         board_tl_x_, board_tl_y_, square_size_);
}

bool BoardReader::auto_calibrate(std::string &detected_board, std::string &detected_pieces)
{
  if (!capture_.capture_region(0, 0, capture_.get_screen_width(), capture_.get_screen_height()))
  {
    std::cerr << "[ChessBot][BoardReader] Failed to capture screen for auto-calibration.\n";
    return false;
  }

  cv::Mat screen = capture_to_mat();
  if (screen.empty())
    return false;

  // 1. Check default config first
  std::string def_board, def_piece;
  bool default_works = false;
  if (theme_manager_.load_default_config(def_board, def_piece))
  {
    if (theme_manager_.load_board_theme(def_board))
    {
      cv::Mat corner_template = theme_manager_.get_board_corner_template();
      cv::Mat result;
      cv::matchTemplate(screen, corner_template, result, cv::TM_CCOEFF_NORMED);
      double minVal, maxVal;
      cv::Point minLoc, maxLoc;
      cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

      if (maxVal >= 0.8)
      {
        default_works = true;
        detected_board = def_board;
        int tw = corner_template.cols;
        square_size_ = (tw >= 300) ? (tw / 8) : tw;
        board_tl_x_ = maxLoc.x;
        board_tl_y_ = maxLoc.y;
        board_br_x_ = board_tl_x_ + (square_size_ * 8);
        board_br_y_ = board_tl_y_ + (square_size_ * 8);
        is_calibrated_ = true;
      }
    }
  }

  // 2. Scan all boards if default failed
  if (!default_works)
  {
    std::vector<std::string> boards = theme_manager_.get_available_boards();
    double best_board_score = 0.0;
    std::string best_board_name;
    cv::Point best_board_loc;
    int best_square_size = 0;

    for (const auto &b : boards)
    {
      if (!theme_manager_.load_board_theme(b)) continue;
      cv::Mat corner = theme_manager_.get_board_corner_template();
      if (corner.empty()) continue;

      cv::Mat result;
      cv::matchTemplate(screen, corner, result, cv::TM_CCOEFF_NORMED);
      double minVal, maxVal;
      cv::Point minLoc, maxLoc;
      cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

      if (maxVal > best_board_score)
      {
        best_board_score = maxVal;
        best_board_name = b;
        best_board_loc = maxLoc;
        int tw = corner.cols;
        best_square_size = (tw >= 300) ? (tw / 8) : tw;
      }
    }

    if (best_board_score >= 0.8)
    {
      detected_board = best_board_name;
      square_size_ = best_square_size;
      board_tl_x_ = best_board_loc.x;
      board_tl_y_ = best_board_loc.y;
      board_br_x_ = board_tl_x_ + (square_size_ * 8);
      board_br_y_ = board_tl_y_ + (square_size_ * 8);
      is_calibrated_ = true;
      theme_manager_.load_board_theme(best_board_name); // ensure it's the active one
    }
    else
    {
      return false; // Could not find any board
    }
  }

  // 3. Scan all pieces (if default doesn't work, or we just want to ensure we have pieces)
  // For simplicity, we rescan pieces if default failed or if we didn't check piece correctness yet.
  if (!default_works)
  {
    std::vector<std::string> pieces = theme_manager_.get_available_pieces();
    double best_piece_score = 0.0;
    std::string best_piece_name;

    for (const auto &p : pieces)
    {
      if (!theme_manager_.load_piece_theme(p)) continue;
      
      // Calculate an average confidence score for this piece theme by running a quick read_board
      double total_confidence = 0.0;
      int pieces_found = 0;

      for (int rank = 0; rank < 8; rank++)
      {
        for (int file = 0; file < 8; file++)
        {
          int sx = file * square_size_;
          int sy = rank * square_size_;
          cv::Rect square_roi(board_tl_x_ + sx, board_tl_y_ + sy, square_size_, square_size_);
          if (square_roi.x < 0 || square_roi.y < 0 ||
              square_roi.x + square_roi.width > screen.cols ||
              square_roi.y + square_roi.height > screen.rows) continue;

          cv::Mat square_img = screen(square_roi);
          
          double highest_sq_val = 0.0;
          for (int pid = 1; pid <= 12; pid++)
          {
            cv::Mat templ = theme_manager_.get_template(pid);
            cv::Mat mask = theme_manager_.get_mask(pid);
            if (templ.empty()) continue;
            
            // Prevent crash if template is larger than ROI
            cv::Mat search_templ = templ;
            cv::Mat search_mask = mask;
            if (search_templ.cols > square_img.cols || search_templ.rows > square_img.rows) {
              cv::resize(search_templ, search_templ, cv::Size(std::min(search_templ.cols, square_img.cols), std::min(search_templ.rows, square_img.rows)));
              if (!search_mask.empty()) {
                cv::resize(search_mask, search_mask, cv::Size(std::min(search_mask.cols, square_img.cols), std::min(search_mask.rows, square_img.rows)));
              }
            }

            cv::Mat result;
            if (!search_mask.empty()) {
              cv::matchTemplate(square_img, search_templ, result, cv::TM_CCOEFF_NORMED, search_mask);
            } else {
              cv::matchTemplate(square_img, search_templ, result, cv::TM_CCOEFF_NORMED);
            }
            double minV, maxV;
            cv::minMaxLoc(result, &minV, &maxV);
            if (maxV > highest_sq_val) highest_sq_val = maxV;
          }
          if (highest_sq_val > 0.7)
          {
            total_confidence += highest_sq_val;
            pieces_found++;
          }
        }
      }

      double avg_score = pieces_found > 0 ? (total_confidence / pieces_found) * pieces_found : 0;
      if (avg_score > best_piece_score)
      {
        best_piece_score = avg_score;
        best_piece_name = p;
      }
    }

    if (!best_piece_name.empty())
    {
      detected_pieces = best_piece_name;
      theme_manager_.load_piece_theme(best_piece_name);
      theme_manager_.save_default_config(detected_board, detected_pieces);
    }
    else
    {
      // fallback
      detected_pieces = "unknown";
    }
  }
  else
  {
    detected_pieces = def_piece;
    theme_manager_.load_piece_theme(def_piece);
  }

  printf("[ChessBot][BoardReader] Auto-Calibrated: board at (%d, %d), square %d. Theme: %s/%s\n",
         board_tl_x_, board_tl_y_, square_size_, detected_board.c_str(), detected_pieces.c_str());

  return true;
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

void BoardReader::get_square_center(int file, int rank,
                                    int &screen_x, int &screen_y) const
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

Board BoardReader::read_board()
{
  Board board = {};
  if (!is_calibrated_)
    return board;

  // Increase margin to 16 to allow for up to 16 pixels of manual calibration error or window shift
  int margin = 16;
  if (!capture_.capture_region(board_tl_x_ - margin, board_tl_y_ - margin, 
                               (square_size_ * 8) + (margin * 2), 
                               (square_size_ * 8) + (margin * 2)))
    return board;

  cv::Mat screen = capture_to_mat();
  if (screen.empty())
    return board;

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
      if (roi_x + roi_w > screen.cols) roi_w = screen.cols - roi_x;
      if (roi_y + roi_h > screen.rows) roi_h = screen.rows - roi_y;

      cv::Rect square_roi(roi_x, roi_y, roi_w, roi_h);
      if (square_roi.width <= 0 || square_roi.height <= 0)
      {
        continue;
      }

      cv::Mat square_img = screen(square_roi);
      
      // DEBUG: dump square image to disk once
      static bool dumped = false;
      if (!dumped && rank == 6 && file == 0) { // e.g., a white pawn on a2
          cv::imwrite("debug_square_" + std::to_string(rank) + "_" + std::to_string(file) + ".png", square_img);
      }
      if (rank == 7 && file == 7) dumped = true;

      double best_match = 1e9; // SQDIFF is lower-is-better
      Piece best_piece = Piece::EMPTY;

      // Loop through pieces 1 to 12
      for (int p = 1; p <= 12; p++)
      {
        cv::Mat templ = theme_manager_.get_template(p);
        cv::Mat mask = theme_manager_.get_mask(p);
        if (templ.empty())
          continue;

        cv::Mat search_templ = templ;
        cv::Mat search_mask = mask;
        if (search_templ.cols != square_size_ || search_templ.rows != square_size_) {
          // Use linear interpolation for the BGR template to keep it smooth
          cv::resize(search_templ, search_templ, cv::Size(square_size_, square_size_), 0, 0, cv::INTER_LINEAR);
          if (!search_mask.empty()) {
            cv::threshold(search_mask, search_mask, 240, 255, cv::THRESH_BINARY);
            cv::resize(search_mask, search_mask, cv::Size(square_size_, square_size_), 0, 0, cv::INTER_NEAREST);
            // Erode mask slightly so the very edge of the piece doesn't dominate the difference
            cv::erode(search_mask, search_mask, cv::Mat(), cv::Point(-1,-1), 1);
          }
        }

        // Apply a slight blur to both the square and the template.
        // This makes the SQDIFF incredibly robust to 1-2 pixel misalignments and anti-aliasing artifacts!
        cv::Mat square_blur, templ_blur;
        cv::GaussianBlur(square_img, square_blur, cv::Size(3, 3), 0);
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

        // A blurred template match with an eroded mask is very stable.
        // Real pieces will score < 0.15. Empty squares will score > 0.30.
        if (minVal < best_match)
        {
          best_match = minVal;
          best_piece = static_cast<Piece>(p);
        }
      }

      if (best_match < 0.25) 
      {
        board[rank][file] = best_piece;
      }
      else
      {
        if (best_match < 0.6) {
           // We expect empty squares to score around 0.30 - 0.50. 
           // If they score under 0.25, they get incorrectly marked as pieces.
           printf("[ChessBot][BoardReader] Square (%d, %d) was borderline empty with SQDIFF %.2f for piece %d\n", rank, file, best_match, (int)best_piece);
        }
        board[rank][file] = Piece::EMPTY;
      }
    }
  }

  return board;
}

char BoardReader::piece_to_fen(Piece p)
{
  switch (p)
  {
  case Piece::WHITE_PAWN: return 'P';
  case Piece::WHITE_KNIGHT: return 'N';
  case Piece::WHITE_BISHOP: return 'B';
  case Piece::WHITE_ROOK: return 'R';
  case Piece::WHITE_QUEEN: return 'Q';
  case Piece::WHITE_KING: return 'K';
  case Piece::BLACK_PAWN: return 'p';
  case Piece::BLACK_KNIGHT: return 'n';
  case Piece::BLACK_BISHOP: return 'b';
  case Piece::BLACK_ROOK: return 'r';
  case Piece::BLACK_QUEEN: return 'q';
  case Piece::BLACK_KING: return 'k';
  default: return '.';
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
