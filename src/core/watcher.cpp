#include "watcher.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/inotify.h>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <mutex>
#include <csignal>

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
        int wd = inotify_add_watch(fd, watch_path.c_str(), IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM);
        if (wd == -1) {
            std::cerr << "Failed to add watch: " << strerror(errno) << '\n';
            close(fd);
            running = false;
            return;
        }
        alignas(inotify_event) char buf[4096];
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
                    EventCallback cb_copy;
                    {
                        std::lock_guard<std::mutex> lock(callback_mutex);
                        cb_copy = callback;
                    }
                    if (cb_copy) {
                        if (event->mask & IN_CREATE) cb_copy(filename, "CREATE");
                        if (event->mask & IN_DELETE) cb_copy(filename, "DELETE");
                        if (event->mask & IN_CLOSE_WRITE) cb_copyfile(name, "MODIFY");
                        if (event->mask & IN_MOVED_TO) cb_copy(filename, "MOVED_TO");
                        if (event->mask & IN_MOVED_FROM) cb_copy(filename, "MOVED_FROM");
                    }
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
    watchr.setCallback([](const std::string& path, const std::string& type) { 
            std::cout << type << " -> " << path << '\n';
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
