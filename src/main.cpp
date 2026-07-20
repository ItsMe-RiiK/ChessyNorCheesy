#include "bot/bot_controller.h"
#include "gui/gui.h"

#include <atomic>
#include <signal.h>

int main(int argc, char** argv) {
    // Ignore SIGPIPE to prevent crash when Stockfish pipe closes
    signal(SIGPIPE, SIG_IGN);

    BotController     bot;
    std::atomic<bool> bot_active(false);

    int status = run_gui(argc, argv, bot, bot_active);

    // Cleanup
    bot.cleanup();

    return status;
}
