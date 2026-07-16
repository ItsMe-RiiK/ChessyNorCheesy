#include "mouse.h"

#include "../rkkdr_common.h"

#include <X11/extensions/XTest.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <random>
#include <thread>
#include <unistd.h>

static std::mt19937 &get_rng()
{
  static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
  return rng;
}

RkkdrMouse::RkkdrMouse() :
    fd_(-1), dev_path_("/dev/rkkdr_mouse"), display_(nullptr), click_delay_min_ms_(30), click_delay_max_ms_(80), move_delay_min_ms_(5),
    move_delay_max_ms_(15), jitter_pixels_(2)
{
}

RkkdrMouse::~RkkdrMouse()
{
  close();
}

bool RkkdrMouse::open()
{
  if (fd_ >= 0)
    return true;

  fd_ = ::open(dev_path_.c_str(), O_WRONLY);
  if (fd_ < 0)
  {
    perror("[ChessBot][Mouse] Failed to open /dev/rkkdr_mouse");
    return false;
  }

  display_ = XOpenDisplay(nullptr);
  if (!display_)
  {
    fprintf(stderr, "[ChessBot][Mouse] Failed to open X11 display\n");
  }

  printf("[ChessBot][Mouse] Device opened successfully.\n");
  return true;
}

void RkkdrMouse::close()
{
  if (display_)
  {
    XCloseDisplay(display_);
    display_ = nullptr;
  }
  if (fd_ >= 0)
  {
    ::close(fd_);
    fd_ = -1;
  }
}

bool RkkdrMouse::is_open() const
{
  return fd_ >= 0;
}

bool RkkdrMouse::send_cmd(uint32_t type, uint32_t code, int32_t value, int32_t extra)
{
  if (fd_ < 0)
    return false;

  struct rkkdr_mouse_cmd cmd;
  memset(&cmd, 0, sizeof(cmd));
  cmd.type = type;
  cmd.code = code;
  cmd.value = value;
  cmd.extra = extra;

  ssize_t written = write(fd_, &cmd, sizeof(cmd));
  if (written != sizeof(cmd))
  {
    perror("[ChessBot][Mouse] Failed to write command");
    return false;
  }

  return true;
}

int RkkdrMouse::random_range(int min_val, int max_val)
{
  if (min_val >= max_val)
    return min_val;
  std::uniform_int_distribution<int> dist(min_val, max_val);
  return dist(get_rng());
}

void RkkdrMouse::random_delay(int min_ms, int max_ms)
{
  int ms = random_range(min_ms, max_ms);
  if (ms > 0)
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

bool RkkdrMouse::move_to(int x, int y)
{
  // Add small random jitter for human-like behavior
  if (jitter_pixels_ > 0)
  {
    x += random_range(-jitter_pixels_, jitter_pixels_);
    y += random_range(-jitter_pixels_, jitter_pixels_);
  }

  // Purely use the kernel driver for absolute positioning.
  // Mixing xdotool and kernel ABS commands causes the compositor to desync and snap to 0,0.
  return send_cmd(RKKDR_MOUSE_ABS, 0, x, y);
}

bool RkkdrMouse::click(int x, int y)
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

bool RkkdrMouse::right_click(int x, int y)
{
  if (!move_to(x, y))
    return false;

  random_delay(move_delay_min_ms_, move_delay_max_ms_);

  if (!button_press(BTN_RIGHT))
    return false;

  random_delay(click_delay_min_ms_, click_delay_max_ms_);

  return button_release(BTN_RIGHT);
}

bool RkkdrMouse::drag(int from_x, int from_y, int to_x, int to_y)
{
  // 1. Move to the source position
  if (!move_to(from_x, from_y))
    return false;

  random_delay(50, 100);

  // 2. FOCUS CLICK: We must do a full press and release first.
  // If the user clicked "Start Bot" in the GTK window, the browser lost focus.
  button_press(BTN_LEFT);
  random_delay(20, 40);
  button_release(BTN_LEFT);

  random_delay(150, 200);

  // 3. ACTUAL DRAG: Now we know the browser is focused. Grab the piece.
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

    // We explicitly call send_cmd directly here to avoid jitter stacking
    send_cmd(RKKDR_MOUSE_ABS, 0, cur_x, cur_y);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  // Ensure we reach the exact destination
  send_cmd(RKKDR_MOUSE_ABS, 0, to_x, to_y);
  random_delay(100, 150);

  // 5. Release hardware button to drop the piece
  if (!button_release(BTN_LEFT))
    return false;

  return true;
}

bool RkkdrMouse::button_press(uint32_t button_code)
{
  return send_cmd(RKKDR_MOUSE_BTN, button_code, 1, 0);
}

bool RkkdrMouse::button_release(uint32_t button_code)
{
  return send_cmd(RKKDR_MOUSE_BTN, button_code, 0, 0);
}

bool RkkdrMouse::scroll(int vertical, int horizontal)
{
  return send_cmd(RKKDR_MOUSE_SCROLL, 0, vertical, horizontal);
}

void RkkdrMouse::set_click_delay_ms(int min_ms, int max_ms)
{
  click_delay_min_ms_ = min_ms;
  click_delay_max_ms_ = max_ms;
}

void RkkdrMouse::set_move_delay_ms(int min_ms, int max_ms)
{
  move_delay_min_ms_ = min_ms;
  move_delay_max_ms_ = max_ms;
}

void RkkdrMouse::set_jitter_pixels(int max_jitter)
{
  jitter_pixels_ = max_jitter;
}
