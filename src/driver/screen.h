#ifndef CHESSBOT_SCREEN_H
#define CHESSBOT_SCREEN_H

#include <cstdint>
#include <string>

/*
 * RkkdrScreen — Userspace wrapper for /dev/rkkdr_screen
 *
 * Reads display metadata (resolution, bpp) from the RKKDR kernel driver.
 * Used for coordinate validation and screen dimension queries.
 */
struct ScreenInfo
{
  uint32_t width;
  uint32_t height;
  uint32_t virtual_width;
  uint32_t virtual_height;
  uint32_t bpp;
  uint32_t line_length;
  uint64_t fb_size;
};

class RkkdrScreen
{
public:
  RkkdrScreen();
  ~RkkdrScreen();

  bool open();
  void close();
  bool is_open() const;

  // Read screen info from kernel driver
  bool get_info(ScreenInfo &info);

  // Convenience getters
  int get_width();
  int get_height();

private:
  int fd_;
  std::string dev_path_;
  ScreenInfo cached_info_;
  bool info_valid_;
};

#endif /* CHESSBOT_SCREEN_H */
