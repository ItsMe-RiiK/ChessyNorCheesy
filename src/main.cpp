#include "bot/bot_controller.h"
#include "chess/stockfish.h"
#include "gui/gui.h"

#include <atomic>
#include <signal.h>
#include <string.h>

static int test_stockfish() {
    printf("=== Stockfish Communication Test ===\n");

    StockfishEngine sf;

    if (!sf.start())
    {
        fprintf(stderr, "FAIL: Could not start Stockfish\n");
        return 1;
    }

    printf("OK: Stockfish started: %s\n", sf.get_engine_info().c_str());

    // Test with starting position
    sf.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::string move = sf.get_best_move(10);

    if (move.empty())
    {
        fprintf(stderr, "FAIL: No best move returned\n");
        sf.stop();
        return 1;
    }

    printf("OK: Best move from startpos (depth 10): %s\n", move.c_str());
    printf("OK: Eval: %d cp\n", sf.get_last_score());
    printf("OK: PV: %s\n", sf.get_last_pv().c_str());

    // Test with a tactical position (Italian Game: 1.e4 e5 2.Nf3 Nc6 3.Bc4 Nf6)
    sf.set_position("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");
    move = sf.get_best_move(10);
    printf("OK: Italian Game (depth 10): %s (eval: %d cp)\n", move.c_str(), sf.get_last_score());

    sf.stop();
    printf("=== All Stockfish tests PASSED ===\n");
    return 0;
}

/* ============================================================
 * Main
 * ============================================================ */

int main(int argc, char** argv) {
    // Ignore SIGPIPE to prevent crash when Stockfish pipe closes
    signal(SIGPIPE, SIG_IGN);

    // Handle help argument
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0)
        {
            printf("ChessyNotCheesy — Chess.com Automation\n\n");
            printf("Usage: %s [OPTIONS]\n\n", argv[0]);
            printf("Options:\n");
            printf("  --help             Show this help\n\n");
            printf("Hotkey: ` (backtick) to toggle bot on/off\n");
            return 0;
        }
    }

    BotController     bot;
    std::atomic<bool> bot_active(false);

    int status = run_gui(argc, argv, bot, bot_active);

    // Cleanup
    bot.cleanup();

    return status;
}
