#ifndef CHESSBOT_HTTP_SERVER_H
#define CHESSBOT_HTTP_SERVER_H

#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>

class BotHttpServer {
public:
    BotHttpServer();
    ~BotHttpServer();

    // Start the server on a specific port
    bool start(int port = 8080);
    void stop();

    // Set callback when a FEN is received
    void set_on_fen_received(std::function<void(const std::string&)> callback);

    // Check if we have a new FEN in the queue
    bool has_new_fen() const;
    std::string pop_fen();

private:
    void server_loop(int port);

    std::atomic<bool> running_;
    std::thread server_thread_;
    std::function<void(const std::string&)> on_fen_received_;

    mutable std::mutex fen_mutex_;
    std::queue<std::string> fen_queue_;
};

#endif
