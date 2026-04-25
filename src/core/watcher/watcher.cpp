#include "watcher.hpp"
#include "event_processor.hpp"
#include "watch_registry.hpp"
#include "move_tracker.hpp"
#include "debounce_buffer.hpp"
#include "utils.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/inotify.h>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <mutex>
#include <csignal>
#include <thread>

Watcher::Watcher(std::string path_to_watch) : watch_path(std::move(path_to_watch)) {}
void Watcher::setCallback(EventCallback cb) {
    // может вызываться из другого потока + защищаем mutex'ом во избежании ошибок с двойным использованием
    std::lock_guard<std::mutex> lock(callback_mutex);
    callback = std::move(cb);
}
Watcher::~Watcher() { stop(); }
void Watcher::stop() {
    // exchange(false) возвращает старое значение
    if (!running.exchange(false)) return;
    // ждем завершения worker потока
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
        WatchRegistry wr(fd);
        wr.addWatchRecursive(watch_path);
        MoveTracker mt;
        DebounceBuffer db;
        alignas(inotify_event) char buf[4096];
        EventProcessor ep(wr, mt, db, nullptr);
        while (running) {
            EventCallback cb_copy;
            {
                std::lock_guard<std::mutex> lock(callback_mutex);
                cb_copy = callback;
            }
            ep.setCallback(cb_copy);
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
                std::string base_path = wr.getPath(event->wd);
                ep.process(event, base_path);
                i += sizeof(inotify_event) + event->len;
            }
            ep.flush();
        }
        wr.cleanup();
        close(fd);
    });
}


std::atomic<bool> shutdown_requested{false};

void handler(int) {
    // что делать при ctrl+c
    // ТУТ НЕЛЬЗЯ ДЕЛАТЬ СЛОЖНУЮ ЛОГИКУ ЗАПРЕЩАЮ
    shutdown_requested = true;
}

void handle_event(Indexer& indexer, const std::string& path, const std::string& type) {
    if (type == "RENAMED") {
        auto pos = path.find('|');
        if (pos == std::string::npos) return;

        std::string old_path = path.substr(0, pos);
        std::string new_path = path.substr(pos + 1);
        indexer.rename(old_path, new_path);
        Change c;
        c.file = path;
        storage_append({c});
        return;
    }
    if (type == "DELETED") {
        indexer.remove(path);
        Change c;
        c.file = path;
        storage_append({c});
        return;
    }
    auto changes = indexer.process(path);
    if (!changes.empty()) {
        storage_append(changes);
    }
}

int main() {
    Watcher watchr("./");
    struct sigaction sa{};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    // безопасная регисрация сигнала
    sigaction(SIGINT, &sa, nullptr);
    Indexer indexer;
    // самая мозгоебская часть - связующее звено между вотчером и индексером
    watchr.setCallback([&indexer](const std::string& path, const std::string& type) {
            // вотчер сообщает об изменении , а индексер вычисляет различия
            handle_event(indexer, path, type);
    });
    watchr.start();
    std::cout << "[Main] Watcher is running. Press Ctrl+C to stop.\n";
    // главный поток ждет сигнал завершения
    while (!shutdown_requested) {
        std::this_thread::sleep_for(std::chrono::seconds(200));
    }
    std::cout << "\n[Main] Shutting down...\n";
    watchr.stop();
    return 0;
}
