#include "theme_manager.h"

#include <dirent.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>

ThemeManager::ThemeManager()
{
}

bool ThemeManager::load_board_theme(const std::string &board_name)
{
  std::string corner_path = "themes/chessboard/" + board_name + "/board_corner.png";
  cv::Mat corner = cv::imread(corner_path, cv::IMREAD_COLOR);
  
  if (corner.empty())
  {
    std::cerr << "[ThemeManager] Failed to load board corner template: " << corner_path << "\n";
    return false;
  }
  
  board_corner_ = corner;
  active_board_ = board_name;
  return true;
}

bool ThemeManager::load_piece_theme(const std::string &piece_name)
{
  piece_templates_.clear();

  std::map<int, std::string> piece_files = {
      {1, "wp.png"}, {2, "wn.png"}, {3, "wb.png"},
      {4, "wr.png"}, {5, "wq.png"}, {6, "wk.png"},
      {7, "bp.png"}, {8, "bn.png"}, {9, "bb.png"},
      {10, "br.png"}, {11, "bq.png"}, {12, "bk.png"}
  };

  bool all_loaded = true;
  std::string base_path = "themes/pieces/" + piece_name + "/";

  for (const auto &kv : piece_files)
  {
    std::string file_path = base_path + kv.second;
    cv::Mat img = cv::imread(file_path, cv::IMREAD_UNCHANGED);
    if (img.empty())
    {
      std::cerr << "[ThemeManager] Failed to load piece template: " << file_path << "\n";
      all_loaded = false;
    }
    else
    {
      if (img.channels() == 4)
      {
        std::vector<cv::Mat> channels;
        cv::split(img, channels);
        cv::Mat bgr;
        cv::merge(std::vector<cv::Mat>{channels[0], channels[1], channels[2]}, bgr);
        piece_templates_[kv.first] = bgr;
        
        cv::Mat mask = channels[3];
        // We use the exact mask without heavy erosion so we preserve the dark outlines.
        // Preserving outlines ensures empty light squares aren't falsely detected as white pieces.
        piece_masks_[kv.first] = mask;
      }
      else
      {
        piece_templates_[kv.first] = img;
        piece_masks_[kv.first] = cv::Mat(); // No mask
      }
    }
  }

  if (all_loaded || !piece_templates_.empty())
  {
    active_piece_ = piece_name;
    return all_loaded; // true if perfect, false if missing some but we still load what we can
  }

  return false;
}

bool ThemeManager::save_default_config(const std::string &board_name, const std::string &piece_name) const
{
  std::ofstream out("themes/default.cfg");
  if (!out.is_open()) return false;
  out << board_name << "\n" << piece_name << "\n";
  return true;
}

bool ThemeManager::load_default_config(std::string &out_board_name, std::string &out_piece_name) const
{
  std::ifstream in("themes/default.cfg");
  if (!in.is_open()) return false;
  
  if (std::getline(in, out_board_name) && std::getline(in, out_piece_name))
  {
    return true;
  }
  return false;
}

cv::Mat ThemeManager::get_template(int piece_id) const
{
  auto it = piece_templates_.find(piece_id);
  if (it != piece_templates_.end())
  {
    return it->second;
  }
  return cv::Mat();
}

cv::Mat ThemeManager::get_mask(int piece_id) const
{
  auto it = piece_masks_.find(piece_id);
  if (it != piece_masks_.end())
  {
    return it->second;
  }
  return cv::Mat();
}

cv::Mat ThemeManager::get_board_corner_template() const
{
  return board_corner_;
}

std::vector<std::string> ThemeManager::get_available_boards() const
{
  return scan_directory("themes/chessboard");
}

std::vector<std::string> ThemeManager::get_available_pieces() const
{
  return scan_directory("themes/pieces");
}

std::vector<std::string> ThemeManager::scan_directory(const std::string &path) const
{
  std::vector<std::string> dirs;
  DIR *dir = opendir(path.c_str());
  if (!dir)
    return dirs;

  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL)
  {
    std::string name = ent->d_name;
    if (name == "." || name == "..")
      continue;

    std::string full_path = path + "/" + name;
    struct stat st;
    if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    {
      dirs.push_back(name);
    }
  }
  closedir(dir);
  return dirs;
}
