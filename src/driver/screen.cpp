#include "screen.h"
#include "../rkkdr_common.h"

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

RkkdrScreen::RkkdrScreen()
    : fd_(-1), dev_path_("/dev/rkkdr_screen"), info_valid_(false)
{
  memset(&cached_info_, 0, sizeof(cached_info_));
}

RkkdrScreen::~RkkdrScreen()
{
  close();
}

bool RkkdrScreen::open()
{
  if (fd_ >= 0)
    return true;

  fd_ = ::open(dev_path_.c_str(), O_RDONLY);
  if (fd_ < 0)
  {
    perror("[ChessBot][Screen] Failed to open /dev/rkkdr_screen");
    return false;
  }

  printf("[ChessBot][Screen] Device opened successfully.\n");
  return true;
}

void RkkdrScreen::close()
{
  if (fd_ >= 0)
  {
    ::close(fd_);
    fd_ = -1;
  }
}

bool RkkdrScreen::is_open() const
{
  return fd_ >= 0;
}

bool RkkdrScreen::get_info(ScreenInfo &info)
{
  if (fd_ < 0)
    return false;

  // Use ioctl to get screen info
  struct rkkdr_screen_info kinfo;
  memset(&kinfo, 0, sizeof(kinfo));

  if (ioctl(fd_, RKKDR_SCREEN_GET_INFO, &kinfo) < 0)
  {
    perror("[ChessBot][Screen] ioctl GET_INFO failed");
    return false;
  }

  info.width = kinfo.width;
  info.height = kinfo.height;
  info.virtual_width = kinfo.virtual_width;
  info.virtual_height = kinfo.virtual_height;
  info.bpp = kinfo.bpp;
  info.line_length = kinfo.line_length;
  info.fb_size = kinfo.fb_size;

  cached_info_ = info;
  info_valid_ = true;

  printf("[ChessBot][Screen] Display: %ux%u @ %ubpp\n",
         info.width, info.height, info.bpp);

  return true;
}

int RkkdrScreen::get_width()
{
  if (!info_valid_)
  {
    ScreenInfo info;
    if (!get_info(info))
      return 1920; // fallback
  }
  return cached_info_.width;
}

int RkkdrScreen::get_height()
{
  if (!info_valid_)
  {
    ScreenInfo info;
    if (!get_info(info))
      return 1080; // fallback
  }
  return cached_info_.height;
}
