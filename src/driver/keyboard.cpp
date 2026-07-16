#include "keyboard.h"

#include "../rkkdr_common.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <thread>
#include <unistd.h>

RkkdrKeyboard::RkkdrKeyboard() : fd_(-1), dev_path_("/dev/rkkdr_keyboard") {}

RkkdrKeyboard::~RkkdrKeyboard()
{
  close();
}

bool RkkdrKeyboard::open()
{
  if (fd_ >= 0)
    return true;

  fd_ = ::open(dev_path_.c_str(), O_WRONLY);
  if (fd_ < 0)
  {
    perror("[ChessBot][Keyboard] Failed to open /dev/rkkdr_keyboard");
    return false;
  }

  printf("[ChessBot][Keyboard] Device opened successfully.\n");
  return true;
}

void RkkdrKeyboard::close()
{
  if (fd_ >= 0)
  {
    ::close(fd_);
    fd_ = -1;
  }
}

bool RkkdrKeyboard::is_open() const
{
  return fd_ >= 0;
}

bool RkkdrKeyboard::send_cmd(uint32_t type, uint32_t code, int32_t value)
{
  if (fd_ < 0)
    return false;

  struct rkkdr_keyboard_cmd cmd;
  memset(&cmd, 0, sizeof(cmd));
  cmd.type = type;
  cmd.code = code;
  cmd.value = value;

  ssize_t written = write(fd_, &cmd, sizeof(cmd));
  if (written != sizeof(cmd))
  {
    perror("[ChessBot][Keyboard] Failed to write command");
    return false;
  }

  return true;
}

bool RkkdrKeyboard::press_key(uint32_t key_code)
{
  return send_cmd(RKKDR_KEYBOARD_KEY, key_code, 1);
}

bool RkkdrKeyboard::release_key(uint32_t key_code)
{
  return send_cmd(RKKDR_KEYBOARD_KEY, key_code, 0);
}

bool RkkdrKeyboard::type_key(uint32_t key_code)
{
  if (!press_key(key_code))
    return false;

  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  return release_key(key_code);
}

bool RkkdrKeyboard::char_to_keycode(char c, uint32_t &key_code, bool &needs_shift)
{
  needs_shift = false;

  // Lowercase letters
  if (c >= 'a' && c <= 'z')
  {
    key_code = KEY_A + (c - 'a');
    return true;
  }

  // Uppercase letters
  if (c >= 'A' && c <= 'Z')
  {
    key_code = KEY_A + (c - 'A');
    needs_shift = true;
    return true;
  }

  // Digits
  if (c >= '1' && c <= '9')
  {
    key_code = KEY_1 + (c - '1');
    return true;
  }
  if (c == '0')
  {
    key_code = KEY_0;
    return true;
  }

  // Common punctuation
  switch (c)
  {
  case ' ':
    key_code = KEY_SPACE;
    return true;
  case '\n':
    key_code = KEY_ENTER;
    return true;
  case '\t':
    key_code = KEY_TAB;
    return true;
  case '-':
    key_code = KEY_MINUS;
    return true;
  case '=':
    key_code = KEY_EQUAL;
    return true;
  case '.':
    key_code = KEY_DOT;
    return true;
  case ',':
    key_code = KEY_COMMA;
    return true;
  case '/':
    key_code = KEY_SLASH;
    return true;
  default:
    return false;
  }
}

bool RkkdrKeyboard::type_string(const std::string &text)
{
  for (char c : text)
  {
    uint32_t key_code;
    bool needs_shift;

    if (!char_to_keycode(c, key_code, needs_shift))
      continue;

    if (needs_shift)
      press_key(KEY_LEFTSHIFT);

    type_key(key_code);

    if (needs_shift)
      release_key(KEY_LEFTSHIFT);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return true;
}
