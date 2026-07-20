#ifndef CHESSY_NOT_CHEESY_THEME_MANAGER_H
#define CHESSY_NOT_CHEESY_THEME_MANAGER_H

#include <map>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

/*
 * ThemeManager
 * Manages separate templates for chessboard and pieces.
 * Handles loading, caching, and iterating through available themes.
 */
class ThemeManager
{
public:
  ThemeManager();
  ~ThemeManager() = default;

  // Load a specific board and piece theme combination
  bool load_board_theme(const std::string &board_name);
  bool load_piece_theme(const std::string &piece_name);

  // Default configuration handling
  bool save_default_config(const std::string &board_name, const std::string &piece_name) const;
  bool load_default_config(std::string &out_board_name, std::string &out_piece_name) const;

  // Get active templates
  cv::Mat get_template(int piece_id) const;
  cv::Mat get_mask(int piece_id) const;
  cv::Mat get_board_corner_template() const;

  // Discover available themes in the file system
  std::vector<std::string> get_available_boards() const;
  std::vector<std::string> get_available_pieces() const;

  // Currently loaded themes
  std::string get_active_board_name() const
  {
    return active_board_;
  }

  std::string get_active_piece_name() const
  {
    return active_piece_;
  }

private:
  std::string active_board_;
  std::string active_piece_;

  std::map<int, cv::Mat> piece_templates_;
  std::map<int, cv::Mat> piece_masks_;
  cv::Mat board_corner_;

  std::vector<std::string> scan_directory(const std::string &path) const;
};

#endif // CHESSY_NOT_CHEESY_THEME_MANAGER_H
