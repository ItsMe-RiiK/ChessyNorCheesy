#ifndef CHESSBOT_MOUSE_H
#define CHESSBOT_MOUSE_H

#include <X11/Xlib.h>
#include <cstdint>
#include <string>

/*
 * RkkdrMouse — Userspace wrapper for /dev/rkkdr_mouse
 *
 * Provides high-level mouse control (absolute move, click, drag)
 * via the RKKDR kernel driver's virtual mouse device and X11 for movement.
 */
class RkkdrMouse
{
public:
  RkkdrMouse();
  ~RkkdrMouse();

  bool open();
  void close();
  bool is_open() const;

  // Absolute positioning — move cursor to exact screen coordinate
  bool move_to(int x, int y);

  // Click at position (move + button press + delay + button release)
  bool click(int x, int y);

  // Right-click at position
  bool right_click(int x, int y);

  // Drag from one position to another (press at src, move to dst, release)
  bool drag(int from_x, int from_y, int to_x, int to_y);

  // Low-level button control
  bool button_press(uint32_t button_code);
  bool button_release(uint32_t button_code);

  // Scroll wheel
  bool scroll(int vertical, int horizontal = 0);

  // Human-like timing configuration
  void set_click_delay_ms(int min_ms, int max_ms);
  void set_move_delay_ms(int min_ms, int max_ms);
  void set_jitter_pixels(int max_jitter);

private:
  int fd_;
  std::string dev_path_;
  Display *display_;

  // Human-like timing
  int click_delay_min_ms_;
  int click_delay_max_ms_;
  int move_delay_min_ms_;
  int move_delay_max_ms_;
  int jitter_pixels_;

  bool send_cmd(uint32_t type, uint32_t code, int32_t value, int32_t extra);
  int random_range(int min_val, int max_val);
  void random_delay(int min_ms, int max_ms);
};

#endif /* CHESSBOT_MOUSE_H */
