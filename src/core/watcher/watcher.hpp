#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <chrono>

struct Config {
    std::vector<std::string> allowed_extensions;
    std::chrono::milliseconds debounce_window{150};
    std::chrono::milliseconds move_timeout{500};
    size_t block_size{5};
    std::string log_file{"codeghost.log"};
    std::string root_path{"./"};
};

class Watcher {
    public:
        explicit Watcher(const Config& conf) : config(std::move(conf)) {};
        ~Watcher();
        void start();
        void stop();
        using EventCallback = std::function<void(const std::string& file_path, const std::string& event_type)>;
        void setCallback(EventCallback cb);
    private:
        Config config;
        std::atomic<bool> running{false};
        EventCallback callback;
        std::mutex callback_mutex;
        std::thread worker;
};
