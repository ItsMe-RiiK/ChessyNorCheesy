#ifndef CHESSBOT_KEYBOARD_H
#define CHESSBOT_KEYBOARD_H

#include <cstdint>
#include <string>

/*
 * RkkdrKeyboard — Userspace wrapper for /dev/rkkdr_keyboard
 *
 * Provides keyboard input simulation via the RKKDR kernel driver's
 * virtual keyboard device.
 */
class RkkdrKeyboard
{
public:
  RkkdrKeyboard();
  ~RkkdrKeyboard();

  bool open();
  void close();
  bool is_open() const;

  // Press a key (key down event)
  bool press_key(uint32_t key_code);

  // Release a key (key up event)
  bool release_key(uint32_t key_code);

  // Type a key (press + delay + release)
  bool type_key(uint32_t key_code);

  // Type a string of characters
  bool type_string(const std::string &text);

private:
  int fd_;
  std::string dev_path_;

  bool send_cmd(uint32_t type, uint32_t code, int32_t value);

  // Map ASCII character to Linux keycode + shift state
  static bool char_to_keycode(char c, uint32_t &key_code, bool &needs_shift);
};

#endif /* CHESSBOT_KEYBOARD_H */
