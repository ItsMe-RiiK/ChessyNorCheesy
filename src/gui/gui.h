#ifndef CHESSBOT_GUI_H
#define CHESSBOT_GUI_H

#include "../bot/bot_controller.h"

#include <atomic>

int run_gui(int argc, char **argv, BotController &bot, std::atomic<bool> &bot_active);

#endif /* CHESSBOT_GUI_H */
