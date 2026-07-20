#ifndef CHESSY_NOT_CHEESY_SCREEN_CAPTURE_H
#define CHESSY_NOT_CHEESY_SCREEN_CAPTURE_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

// X11 defines macros that conflict with standard C++ and OpenCV
#undef Status
#undef Success
#undef None
#undef Bool
#undef True
#undef False
#include <cstdint>
#include <sys/shm.h>

/*
 * ScreenCapture — Fast X11 screen capture using XShm (shared memory)
 *
 * Captures screen regions at high speed (~1-2ms per frame) by using
 * X11's shared memory extension. This avoids the overhead of
 * XGetImage which copies pixels over the X11 socket.
 */

struct Pixel
{
  uint8_t r, g, b;
};

class ScreenCapture
{
public:
  ScreenCapture();
  ~ScreenCapture();

  // Initialize X11 connection and shared memory
  bool init();
  void cleanup();

  // Capture a region of the screen into the internal buffer
  // Returns pointer to pixel data (BGRA format, row-major)
  bool capture_region(int x, int y, int width, int height);

  // Get a single pixel color at screen coordinates
  Pixel get_pixel(int x, int y);

  // Get pixel from the last captured region (relative to capture origin)
  Pixel get_captured_pixel(int rel_x, int rel_y) const;

  // Get raw buffer from last capture
  const uint8_t *get_buffer() const;
  int get_capture_width() const;
  int get_capture_height() const;

  // Screen dimensions
  int get_screen_width() const;
  int get_screen_height() const;
  int get_bytes_per_line() const;

private:
  Display *display_;
  Window root_;
  int screen_width_;
  int screen_height_;

  // XShm resources
  XImage *ximage_;
  XShmSegmentInfo shm_info_;
  bool shm_attached_;

  // Current capture dimensions
  int cap_x_, cap_y_;
  int cap_width_, cap_height_;

  void free_shm();
  bool alloc_shm(int width, int height);
};

#endif /* CHESSY_NOT_CHEESY_SCREEN_CAPTURE_H */
