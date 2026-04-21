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

namespace fs = std::filesystem;
bool shouldIgnoreFile(const std::string& name) {
    // фильтрация мусорных файлов, в редакторах кода при создании/записи и тд также создаются временные файлы, чтобы их исключить нужна эта функция
    // тут игнорируются временные файлы и скрытые файлы
    if (name.empty()) return true;
    if (name.starts_with(".#")) return true;
    if (name.ends_with("~") || name.ends_with(".swp") || name.ends_with(".swo") || name.ends_with(".tmp")) return true;
    if (name[0] == '.') return true;
    return false;
}

void add_watch(int fd, const std::string& path, std::unordered_map<int, std::string>& wd_to_path) {
    for (const auto& [_, p] : wd_to_path) {
        if (p == path) return;
    }
    int wd = inotify_add_watch(fd, path.c_str(), IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO | IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM);
    if (wd >= 0) {
        wd_to_path[wd] = path;
    } else {
        std::cerr << "Failed to add watch for "
                  << path << ": " << strerror(errno) << '\n';
    }
}

void add_watch_recursive(int fd, const std::string& root, std::unordered_map<int, std::string>& wd_to_path) {
    add_watch(fd, root, wd_to_path);
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(root, ec)) {
        if (ec) continue;
        if (!entry.is_directory()) continue;
        const std::string path = entry.path().string();
        auto name = entry.path().filename().string();
        if (shouldIgnoreFile(name)) continue;
        add_watch(fd, path, wd_to_path);
    }
}

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
        std::unordered_map<int, std::string> wd_to_path;
        std::unordered_map<uint32_t, Watcher::PendingMove> pending_moves;
        add_watch_recursive(fd, watch_path, wd_to_path);
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
            ssize_t i =0;
            while (i < len) {
                auto* event = reinterpret_cast<inotify_event*>(buf + i);
                auto it_wd = wd_to_path.find(event->wd);
                if (it_wd == wd_to_path.end()) {
                    i += sizeof(inotify_event) + event->len;
                    continue;
                }
                std::string base_path = it_wd->second;
                if (event->mask & IN_IGNORED) {
                    wd_to_path.erase(it_wd);
                    i += sizeof(inotify_event) + event->len;
                    continue;
                }
                if (event->mask & IN_DELETE_SELF) {
                    auto it = wd_to_path.begin();
                    while (it != wd_to_path.end()) {
                        if (it->second.starts_with(base_path + "/")) {
                            inotify_rm_watch(fd, it->first);
                            it = wd_to_path.erase(it);
                        } else {
                            ++it;
                        }
                    }
                    inotify_rm_watch(fd, event->wd);
                    wd_to_path.erase(event->wd);
                    EventCallback cb_copy;
                    {
                        std::lock_guard<std::mutex> lock(callback_mutex);
                        cb_copy = callback;
                    }
                    if (cb_copy) cb_copy(base_path, "DELETED");
                    i += sizeof(inotify_event) + event->len;
                    continue;
                }
                if (event->len > 0 && (event->mask & IN_ISDIR) && (event->mask & (IN_CREATE | IN_MOVED_TO))) {
                    std::string new_dir = base_path + "/" + event->name;
                    add_watch(fd, new_dir, wd_to_path);
                    std::error_code ec;
                    for (const auto& entry : fs::recursive_directory_iterator(new_dir, ec)) {
                        if (ec) break;
                        if (entry.is_directory()) {
                            add_watch(fd, entry.path().string(), wd_to_path);
                        }
                    }
                    i += sizeof(inotify_event) + event->len;
                    continue;
                }
                if (event->len > 0) {
                    std::string filename = event->name;
                    if (shouldIgnoreFile(filename)) {
                        i += sizeof(inotify_event) + event->len;
                        continue;
                    }
                    if (event->mask & IN_MOVED_FROM) {
                        std::string full_path = base_path + "/" + event->name;
                        pending_moves[event->cookie] = {full_path, std::chrono::steady_clock::now()};
                        i += sizeof(inotify_event) + event->len;
                        continue;
                    }
                    if (event->mask & IN_MOVED_TO) {
                        std::string new_path = base_path + "/" + event->name;
                        EventCallback cb_copy;
                        {
                            std::lock_guard<std::mutex> lock(callback_mutex);
                            cb_copy = callback;
                        }

                        auto it = pending_moves.find(event->cookie);
                        if (it != pending_moves.end()) {
                            std::string old_path = it->second.path;
                            if (cb_copy) cb_copy(old_path + "|" + new_path, "RENAMED");
                            pending_moves.erase(it);
                        } else {
                            if (cb_copy) cb_copy(new_path, "MODIFIED");
                        }
                        i += sizeof(inotify_event) + event->len;
                        continue;
                    }
                    if (event->mask & IN_DELETE) {
                        std::string full_path = base_path + "/" + event->name;
                        EventCallback cb_copy;
                        {
                            std::lock_guard<std::mutex> lock(callback_mutex);
                            cb_copy = callback;
                        }
                        if (cb_copy) cb_copy(full_path, "DELETED");
                        auto it = pending_moves.begin();
                        while (it != pending_moves.end()) {
                            if (it->second.path == full_path) {
                                pending_moves.erase(it);
                                break;
                            }
                            ++it;
                        }
                        i += sizeof(inotify_event) + event->len;
                        continue;
                    }
                    if (!(event->mask & IN_CLOSE_WRITE)) {
                        i += sizeof(inotify_event) + event->len;
                        continue;
                    }
                    std::string full_path = base_path + "/" + filename;
                    auto now = std::chrono::steady_clock::now();
                    auto it = last_event_time.find(full_path);
                    if (it != last_event_time.end() && now - it->second < debounce_window) {
                        i += sizeof(inotify_event) + event->len;
                        continue;
                    }
                    EventCallback cb_copy;
                    {
                        std::lock_guard<std::mutex> lock(callback_mutex);
                        cb_copy = callback;
                    }
                    if (cb_copy) {
                        cb_copy(full_path, "MODIFIED");
                    }
                    last_event_time[full_path] = now;
                }
                i += sizeof(inotify_event) + event->len;
            }
            auto it = pending_moves.begin();
            while (it != pending_moves.end()) {
                if (std::chrono::steady_clock::now() - it->second.ts > std::chrono::milliseconds(500)) {
                    EventCallback cb_copy;
                    {
                        std::lock_guard<std::mutex> lock(callback_mutex);
                        cb_copy = callback;
                    }
                    if (cb_copy) cb_copy(it->second.path, "DELETED");
                    it = pending_moves.erase(it);
                } else {
                    ++it;
                }
            }
        }
        for (const auto& [wd, _] : wd_to_path) {
            inotify_rm_watch(fd, wd);
        }
        close(fd);
    });
}


std::atomic<bool> shutdown_requested{false};

void handler(int) {
    // что делать при ctrl+c
    // ТУТ НЕЛЬЗЯ ДЕЛАТЬ СЛОЖНУЮ ЛОГИКУ ЗАПРЕЩАЮ
    shutdown_requested = true;
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
            if (type == "RENAMED") {
                auto pos = path.find('|');
                std::string old_path = path.substr(0, pos);
                std::string new_path = path.substr(pos + 1);
                indexer.rename(old_path, new_path);
                return;
            }
            if (type == "DELETED") {
                indexer.remove(path);
                std::cout << path << " [" << type << "]\n";
                return;
            }
            auto changes = indexer.process(path);
            for (auto& c : changes) {
                std::cout << c.file << " [" << c.block_index << "]\n";
            }
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
