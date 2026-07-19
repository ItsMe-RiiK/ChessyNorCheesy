#include "mouse.h"

#include <X11/extensions/XTest.h>
#include <chrono>
#include <cstdio>
#include <linux/input.h>
#include <random>
#include <thread>
#include <unistd.h>

static std::mt19937 &get_rng()
{
  static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
  return rng;
}

X11Mouse::X11Mouse() :
    display_(nullptr), click_delay_min_ms_(30), click_delay_max_ms_(80), move_delay_min_ms_(5), move_delay_max_ms_(15), jitter_pixels_(2)
{
}

X11Mouse::~X11Mouse()
{
  close();
}

bool X11Mouse::open()
{
  if (display_)
    return true;

  display_ = XOpenDisplay(nullptr);
  if (!display_)
  {
    fprintf(stderr, "[ChessBot][Mouse] Failed to open X11 display\n");
    return false;
  }

  printf("[ChessBot][Mouse] X11 Display opened successfully.\n");
  return true;
}

void X11Mouse::close()
{
  if (display_)
  {
    XCloseDisplay(display_);
    display_ = nullptr;
  }
}

bool X11Mouse::is_open() const
{
  return display_ != nullptr;
}

int X11Mouse::random_range(int min_val, int max_val)
{
  if (min_val >= max_val)
    return min_val;
  std::uniform_int_distribution<int> dist(min_val, max_val);
  return dist(get_rng());
}

void X11Mouse::random_delay(int min_ms, int max_ms)
{
  int ms = random_range(min_ms, max_ms);
  if (ms > 0)
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

bool X11Mouse::move_to(int x, int y)
{
  // Add small random jitter for human-like behavior
  if (jitter_pixels_ > 0)
  {
    x += random_range(-jitter_pixels_, jitter_pixels_);
    y += random_range(-jitter_pixels_, jitter_pixels_);
  }

  // Use XTest for absolute positioning because the kernel driver ABS_X/ABS_Y
  // often fails to move the core pointer under modern libinput/Wayland/Cinnamon.
  if (display_)
  {
    XTestFakeMotionEvent(display_, -1, x, y, CurrentTime);
    XFlush(display_);
    return true;
  }
  return false;
}

bool X11Mouse::click(int x, int y)
{
  if (!move_to(x, y))
    return false;

  random_delay(move_delay_min_ms_, move_delay_max_ms_);

  // Always use the hardware-level kernel driver for clicks to bypass Wayland restrictions.
  if (!button_press(BTN_LEFT))
    return false;

  random_delay(click_delay_min_ms_, click_delay_max_ms_);

  return button_release(BTN_LEFT);
}

bool X11Mouse::right_click(int x, int y)
{
  if (!move_to(x, y))
    return false;

  random_delay(move_delay_min_ms_, move_delay_max_ms_);

  if (!button_press(BTN_RIGHT))
    return false;

  random_delay(click_delay_min_ms_, click_delay_max_ms_);

  return button_release(BTN_RIGHT);
}

bool X11Mouse::drag(int from_x, int from_y, int to_x, int to_y)
{
  // 1. Move to the source position
  if (!move_to(from_x, from_y))
    return false;

  random_delay(50, 100);

  // 3. ACTUAL DRAG: Grab the piece.
  if (!button_press(BTN_LEFT))
    return false;

  random_delay(50, 80);

  // 4. Interpolate the movement using absolute coordinates
  int dx = to_x - from_x;
  int dy = to_y - from_y;
  int steps = 20;

  for (int i = 1; i <= steps; ++i)
  {
    int cur_x = from_x + (dx * i) / steps;
    int cur_y = from_y + (dy * i) / steps;

    // Use move_to() which leverages XTest.
    // The kernel ABS_X/ABS_Y commands are often ignored by libinput/Wayland/Cinnamon
    // which caused the piece to be clicked but never dragged.
    move_to(cur_x, cur_y);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  // Ensure we reach the exact destination
  move_to(to_x, to_y);
  random_delay(100, 150);

  // 5. Release hardware button to drop the piece
  if (!button_release(BTN_LEFT))
    return false;

  return true;
}

bool X11Mouse::button_press(uint32_t button_code)
{
  if (display_)
  {
    int x11_btn = 1;
    if (button_code == BTN_RIGHT)
      x11_btn = 3;
    else if (button_code == BTN_MIDDLE)
      x11_btn = 2;
    XTestFakeButtonEvent(display_, x11_btn, True, CurrentTime);
    XFlush(display_);
    return true;
  }
  return false;
}

bool X11Mouse::button_release(uint32_t button_code)
{
  if (display_)
  {
    int x11_btn = 1;
    if (button_code == BTN_RIGHT)
      x11_btn = 3;
    else if (button_code == BTN_MIDDLE)
      x11_btn = 2;
    XTestFakeButtonEvent(display_, x11_btn, False, CurrentTime);
    XFlush(display_);
    return true;
  }
  return false;
}

void X11Mouse::set_click_delay_ms(int min_ms, int max_ms)
{
  click_delay_min_ms_ = min_ms;
  click_delay_max_ms_ = max_ms;
}

void X11Mouse::set_move_delay_ms(int min_ms, int max_ms)
{
  move_delay_min_ms_ = min_ms;
  move_delay_max_ms_ = max_ms;
}

void X11Mouse::set_jitter_pixels(int max_jitter)
{
  jitter_pixels_ = max_jitter;
}
