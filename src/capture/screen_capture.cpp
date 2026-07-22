#include "screen_capture.h"

#include <cstdio>
#include <cstring>

ScreenCapture::ScreenCapture() :
    display_(nullptr),
    root_(0),
    screen_width_(0),
    screen_height_(0),
    ximage_(nullptr),
    shm_attached_(false),
    cap_x_(0),
    cap_y_(0),
    cap_width_(0),
    cap_height_(0)
{
  memset(&shm_info_, 0, sizeof(shm_info_));
}

ScreenCapture::~ScreenCapture() { cleanup(); }

bool ScreenCapture::init()
{
  display_ = XOpenDisplay(nullptr);
  if (!display_) {
    fprintf(stderr, "[Capture] Failed to open X11 display\n");
    return false;
  }

  int screen     = DefaultScreen(display_);
  root_          = RootWindow(display_, screen);
  screen_width_  = DisplayWidth(display_, screen);
  screen_height_ = DisplayHeight(display_, screen);

  // Check XShm extension
  if (!XShmQueryExtension(display_)) {
    fprintf(stderr, "[Capture] XShm extension not available\n");
    XCloseDisplay(display_);
    display_ = nullptr;
    return false;
  }

  printf("[Capture] X11 initialized: %dx%d, XShm available\n", screen_width_, screen_height_);

  return true;
}

void ScreenCapture::cleanup()
{
  free_shm();

  if (display_) {
    XCloseDisplay(display_);
    display_ = nullptr;
  }
}

void ScreenCapture::free_shm()
{
  if (ximage_) {
    if (shm_attached_) {
      XShmDetach(display_, &shm_info_);
      shm_attached_ = false;
    }

    XDestroyImage(ximage_);
    ximage_ = nullptr;

    if (shm_info_.shmid >= 0) {
      shmdt(shm_info_.shmaddr);
      shmctl(shm_info_.shmid, IPC_RMID, nullptr);
    }

    memset(&shm_info_, 0, sizeof(shm_info_));
  }
}

bool ScreenCapture::alloc_shm(int width, int height)
{
  if (!display_)
    return false;

  // Free previous allocation if dimensions changed
  if (ximage_ && (cap_width_ != width || cap_height_ != height)) {
    free_shm();
  }

  if (ximage_)
    return true;  // Already allocated with correct size

  int     screen = DefaultScreen(display_);
  int     depth  = DefaultDepth(display_, screen);
  Visual* visual = DefaultVisual(display_, screen);

  ximage_ = XShmCreateImage(display_, visual, depth, ZPixmap, nullptr, &shm_info_, width, height);

  if (!ximage_) {
    fprintf(stderr, "Capture] XShmCreateImage failed\n");
    return false;
  }

  shm_info_.shmid =
    shmget(IPC_PRIVATE, ximage_->bytes_per_line * ximage_->height, IPC_CREAT | 0600);

  if (shm_info_.shmid < 0) {
    fprintf(stderr, "[Capture] shmget failed\n");
    XDestroyImage(ximage_);
    ximage_ = nullptr;
    return false;
  }

  shm_info_.shmaddr = ximage_->data = (char*) shmat(shm_info_.shmid, nullptr, 0);
  shm_info_.readOnly                = false;

  if (!XShmAttach(display_, &shm_info_)) {
    fprintf(stderr, "[Capture] XShmAttach failed\n");
    shmdt(shm_info_.shmaddr);
    shmctl(shm_info_.shmid, IPC_RMID, nullptr);
    XDestroyImage(ximage_);
    ximage_ = nullptr;
    return false;
  }

  // MUST sync with X server so it finishes attaching before we capture
  XSync(display_, false);

  shm_attached_ = true;
  cap_width_    = width;
  cap_height_   = height;

  return true;
}

bool ScreenCapture::capture_region(int x, int y, int width, int height)
{
  if (!display_)
    return false;

  // Clamp to screen bounds
  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;
  if (x + width > screen_width_)
    width = screen_width_ - x;
  if (y + height > screen_height_)
    height = screen_height_ - y;

  if (width <= 0 || height <= 0)
    return false;

  if (!alloc_shm(width, height))
    return false;

  cap_x_ = x;
  cap_y_ = y;

  if (!XShmGetImage(display_, root_, ximage_, x, y, AllPlanes)) {
    fprintf(stderr, "[Capture] XShmGetImage failed\n");
    return false;
  }

  // FIX MEMORY LEAK: XShmGetImage sends an XShmCompletionEvent to the client.
  // Since we never read from this display connection's event queue, Xlib builds up
  // an infinite linked list of events, eventually crashing the system with OOM!
  XSync(display_, false);
  XEvent ev;
  while (XCheckMaskEvent(display_, ~0L, &ev)) {
    // discard
  }

  return true;
}

const uint8_t* ScreenCapture::get_buffer() const
{
  if (!ximage_)
    return nullptr;
  return (const uint8_t*) ximage_->data;
}

int ScreenCapture::get_capture_width() const { return cap_width_; }

int ScreenCapture::get_capture_height() const { return cap_height_; }

int ScreenCapture::get_screen_width() const { return screen_width_; }

int ScreenCapture::get_screen_height() const { return screen_height_; }

int ScreenCapture::get_bytes_per_line() const { return ximage_ ? ximage_->bytes_per_line : 0; }
