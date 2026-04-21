#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <chrono>

class Watcher {
    public:
        struct PendingMove {
            std::string path;
            std::chrono::steady_clock::time_point ts;
        };
        explicit Watcher(std::string path_to_watch);
        ~Watcher();
        void start();
        void stop();
        using EventCallback = std::function<void(const std::string& file_path, const std::string& event_type)>;
        void setCallback(EventCallback cb);
    private:
        std::string watch_path;
        std::atomic<bool> running{false};
        EventCallback callback;
        std::mutex callback_mutex;
        std::thread worker;
};
