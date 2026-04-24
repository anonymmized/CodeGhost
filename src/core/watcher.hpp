#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <chrono>

bool shouldIgnoreFile(const std::string& name);

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

class WatchRegistry {
    public:
        explicit WatchRegistry(int n_fd) : fd(n_fd) {}
        void addWatch(const std::string& path);
        void addWatchRecursive(const std::string& root);
        std::string getPath(int wd);
        void removeSubtree(const std::string& path, int wd);
        void cleanup();
        void remove(int wd);
    private:
        int fd;
        std::unordered_map<int, std::string> wd_to_path;
};

class MoveTracker {
    public:
        void onMovedFrom(uint32_t cookie, const std::string& path);
        std::optional<std::string> onMovedTo(uint32_t cookie, const std::string& new_path);
        template<typename Callback>
        void flush(Callback cb) {
            auto now = std::chrono::steady_clock::now();
            for (auto it = pending.begin(); it != pending.end(); ) {
                if (now - it->second.ts > std::chrono::milliseconds(500)) {
                    cb(it->second.path, "DELETED");
                    it = pending.erase(it);
                } else {
                    ++it;
                }
            }
        }
    private:
        struct PendingMove {
            std::string path;
            std::chrono::steady_clock::time_point ts;
        };
        std::unordered_map<uint32_t, PendingMove> pending;
};

class DebounceBuffer {
    public:
        void touch(const std::string& path);
        template<typename Callback>
        void flush(Callback cb) {
            auto now = std::chrono::steady_clock::now();
            const auto debounce_window = std::chrono::milliseconds(150);
            for (auto it = files.begin(); it != files.end(); ) {
                if (now - it->second.ts > debounce_window) {
                    cb(it->first, "MODIFIED");
                    it = files.erase(it);
                } else {
                    ++it;
                }
            }
        }
    private:
        struct State {
            std::chrono::steady_clock::time_point ts;
        };
        std::unordered_map<std::string, State> files;
};
