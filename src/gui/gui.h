#ifndef CHESSY_NOT_CHEESY_GUI_H
#define CHESSY_NOT_CHEESY_GUI_H

#include "../bot/bot_controller.h"

#include <atomic>

int run_gui(int argc, char **argv, BotController &bot, std::atomic<bool> &bot_active);

#endif /* CHESSY_NOT_CHEESY_GUI_H */
