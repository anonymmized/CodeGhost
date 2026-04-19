#include "watcher.hpp"
#include "indexer.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/inotify.h>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <mutex>
#include <csignal>
#include <unordered_map>
#include <filesystem>
#include <thread>


bool shouldIgnoreFile(const std::string& name) {
    if (name.empty()) return true;
    if (name.starts_with(".#")) return true;
    if (name.ends_with("~") || name.ends_with(".swp") || name.ends_with(".swo") || name.ends_with(".tmp")) return true;
    if (name[0] == '.') return true;
    return false;
}

Watcher::Watcher(std::string path_to_watch) : watch_path(std::move(path_to_watch)) {}
void Watcher::setCallback(EventCallback cb) {
    std::lock_guard<std::mutex> lock(callback_mutex);
    callback = std::move(cb);
}
Watcher::~Watcher() { stop(); }
void Watcher::stop() { 
    if (!running.exchange(false)) return;
    if (worker.joinable()) worker.join();
}

void Watcher::start() {
    if (running.exchange(true)) return;
    worker = std::thread([this]() {
        int fd = inotify_init1(IN_NONBLOCK);
        if (fd == -1) {
            std::cerr << "Failed to initialize inotify: " << strerror(errno) << '\n';
            running = false;
            return;
        }
        int wd = inotify_add_watch(fd, watch_path.c_str(), IN_CLOSE_WRITE);
        if (wd == -1) {
            std::cerr << "Failed to add watch: " << strerror(errno) << '\n';
            close(fd);
            running = false;
            return;
        }
        alignas(inotify_event) char buf[4096];
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_event_time;
        const auto debounce_window = std::chrono::milliseconds(150);
        while (running) {
            ssize_t len = read(fd, buf, sizeof(buf));
            if (len < 0) {
                if (errno == EAGAIN) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
                if (errno == EINTR) continue;
                std::cerr << "read error: " << strerror(errno) << '\n';
                break;
            }
            ssize_t i = 0;
            while (i < len) {
                auto* event = reinterpret_cast<inotify_event*>(buf + i);
                if (event->len > 0) {
                    std::string filename = event->name;
                    if (shouldIgnoreFile(filename)) {
                        i += sizeof(inotify_event) + event->len;
                        continue;
                    }
                    std::string full_path = (std::filesystem::path(watch_path) / filename).string();
                    if (!(event->mask & IN_CLOSE_WRITE)) {
                        i += sizeof(inotify_event) + event->len;
                        continue;
                    }
                    auto now = std::chrono::steady_clock::now();
                    auto it = last_event_time.find(filename);
                    if (it != last_event_time.end()) {
                        if (now - it->second < debounce_window) {
                            i += sizeof(inotify_event) + event->len;
                            continue;
                        }
                    }
                    EventCallback cb_copy;
                    {
                        std::lock_guard<std::mutex> lock(callback_mutex);
                        cb_copy = callback;
                    }

                    if (cb_copy) {
                        cb_copy(full_path, "MODIFIED");
                    }
                    last_event_time[filename] = now;
                }
                i += sizeof(inotify_event) + event->len;
            }
        }
        inotify_rm_watch(fd, wd);
        close(fd);
    });
}

std::atomic<bool> shutdown_requested{false};

void handler(int) {
    shutdown_requested = true;
}

int main() {
    Watcher watchr("./");
    struct sigaction sa{};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    Indexer indexer;
    watchr.setCallback([&indexer](const std::string& path, const std::string& type) { 
            auto changes = indexer.process(path);
            for (auto& c : changes) {
                std::cout << c.file << " [" << c.block_index << "]\n";
            }
    });
    watchr.start();
    std::cout << "[Main] Watcher is running. Press Ctrl+C to stop.\n";
    while (!shutdown_requested) {
        std::this_thread::sleep_for(std::chrono::seconds(200));
    }
    std::cout << "\n[Main] Shutting down...\n";
    watchr.stop();
    return 0;
}
