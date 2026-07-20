#ifndef CHESSY_NOT_CHEESY_MOUSE_H
#define CHESSY_NOT_CHEESY_MOUSE_H

#include <X11/Xlib.h>
#include <cstdint>

/*
 * X11Mouse — Userspace wrapper for X11 mouse injection
 *
 * Provides high-level mouse control (absolute move, click, drag)
 * using the X11 XTest extension.
 */
class X11Mouse
{
public:
  X11Mouse();
  ~X11Mouse();

  bool open();
  void close();

  // Absolute positioning — move cursor to exact screen coordinate
  bool move_to(int x, int y);

  // Click at position (move + button press + delay + button release)
  bool click(int x, int y);

  // Drag from one position to another (press at src, move to dst, release)
  bool drag(int from_x, int from_y, int to_x, int to_y);

  // Low-level button control
  bool button_press(uint32_t button_code);
  bool button_release(uint32_t button_code);

  // Human-like timing configuration
  void set_click_delay_ms(int min_ms, int max_ms);
  void set_move_delay_ms(int min_ms, int max_ms);
  void set_jitter_pixels(int max_jitter);

private:
  Display *display_;

  // Human-like timing
  int click_delay_min_ms_;
  int click_delay_max_ms_;
  int move_delay_min_ms_;
  int move_delay_max_ms_;
  int jitter_pixels_;
  int random_range(int min_val, int max_val);
  void random_delay(int min_ms, int max_ms);
};

#endif /* CHESSY_NOT_CHEESY_MOUSE_H */
