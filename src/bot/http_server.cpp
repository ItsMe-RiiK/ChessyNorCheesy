#include "http_server.h"
#include "../httplib.h"
#include <iostream>

BotHttpServer::BotHttpServer() : running_(false) {}

BotHttpServer::~BotHttpServer() {
    stop();
}

bool BotHttpServer::start(int port) {
    if (running_) return true;
    running_ = true;
    server_thread_ = std::thread(&BotHttpServer::server_loop, this, port);
    return true;
}

void BotHttpServer::stop() {
    running_ = false;
    if (server_thread_.joinable()) {
        server_thread_.detach(); 
    }
}

void BotHttpServer::set_on_fen_received(std::function<void(const std::string&)> callback) {
    on_fen_received_ = callback;
}

bool BotHttpServer::has_new_fen() const {
    std::lock_guard<std::mutex> lock(fen_mutex_);
    return !fen_queue_.empty();
}

std::string BotHttpServer::pop_fen() {
    std::lock_guard<std::mutex> lock(fen_mutex_);
    if (fen_queue_.empty()) return "";
    std::string fen = fen_queue_.front();
    fen_queue_.pop();
    return fen;
}

void BotHttpServer::server_loop(int port) {
    httplib::Server svr;

    svr.Options("/update_fen", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
    });

    svr.Post("/update_fen", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::string fen = req.body;
        
        {
            std::lock_guard<std::mutex> lock(fen_mutex_);
            fen_queue_.push(fen);
        }

        if (on_fen_received_) {
            on_fen_received_(fen);
        }

        res.set_content("OK", "text/plain");
    });

    std::cout << "[ChessBot][HTTP] Starting server on port " << port << "...\n";
    svr.listen("127.0.0.1", port);
    std::cout << "[ChessBot][HTTP] Server stopped.\n";
}
