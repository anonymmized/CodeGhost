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

std::string WatchRegistry::getPath(int wd) {
    auto it_wd = wd_to_path.find(wd);
    if (it_wd == wd_to_path.end()) {
        return "";
    }
    return it_wd->second;
}

void WatchRegistry::addWatch(const std::string& path) {
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

void WatchRegistry::remove(int wd) {
    inotify_rm_watch(fd, wd);
    wd_to_path.erase(wd);
}

void WatchRegistry::addWatchRecursive(const std::string& root) {
    addWatch(root);
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(root, ec)) {
        if (ec) continue;
        if (!entry.is_directory()) continue;
        const std::string path = entry.path().string();
        auto name = entry.path().filename().string();
        if (shouldIgnoreFile(name)) continue;
        addWatch(path);
    }
}

void WatchRegistry::removeSubtree(const std::string& path, int wd) {
    remove(wd);
    auto it = wd_to_path.begin();
    while (it != wd_to_path.end()) {
        if (it->second.starts_with(path + "/")) {
            inotify_rm_watch(fd, it->first);
            it = wd_to_path.erase(it);
        } else {
            ++it;
        }
    }
}

void WatchRegistry::cleanup() {
    for (const auto& [wd, _] : wd_to_path) {
        inotify_rm_watch(fd, wd);
    }
    wd_to_path.clear();
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
        std::unordered_map<uint32_t, Watcher::PendingMove> pending_moves;
        struct EventState {
            std::chrono::steady_clock::time_point last_time;
        };
        std::unordered_map<std::string, EventState> file_events;
        WatchRegistry wr(fd);
        wr.addWatchRecursive(watch_path);
        alignas(inotify_event) char buf[4096];
        const auto debounce_window = std::chrono::milliseconds(150);
        while (running) {
            EventCallback cb_copy;
            {
                std::lock_guard<std::mutex> lock(callback_mutex);
                cb_copy = callback;
            }
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
                std::string base_path = wr.getPath(event->wd);
                if (base_path.empty()) {
                    i += sizeof(inotify_event) + event->len;
                    continue;
                }
                if (event->mask & IN_IGNORED) {
                    wr.remove(event->wd);
                    i += sizeof(inotify_event) + event->len;
                    continue;
                }
                if (event->mask & IN_DELETE_SELF) {
                    wr.removeSubtree(base_path, event->wd);
                    if (cb_copy) cb_copy(base_path, "DELETED");
                    i += sizeof(inotify_event) + event->len;
                    continue;
                }
                if (event->len > 0 && (event->mask & IN_ISDIR) && (event->mask & (IN_CREATE | IN_MOVED_TO))) {
                    std::string new_dir = base_path + "/" + event->name;
                    wr.addWatch(new_dir);
                    std::error_code ec;
                    for (const auto& entry : fs::recursive_directory_iterator(new_dir, ec)) {
                        if (ec) break;
                        if (entry.is_directory()) {
                            wr.addWatch(entry.path().string());
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
                        auto it = pending_moves.find(event->cookie);
                        if (it != pending_moves.end()) {
                            std::string old_path = it->second.path;
                            if (cb_copy) cb_copy(old_path + "|" + new_path, "RENAMED");
                            pending_moves.erase(it);
                        } else {
                            auto now = std::chrono::steady_clock::now();
                            file_events[new_path].last_time = now;
                        }
                        i += sizeof(inotify_event) + event->len;
                        continue;
                    }
                    if (event->mask & IN_DELETE) {
                        std::string full_path = base_path + "/" + event->name;
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
                    auto& st = file_events[full_path];
                    st.last_time = now;
                }
                i += sizeof(inotify_event) + event->len;
            }
            auto it = pending_moves.begin();
            while (it != pending_moves.end()) {
                if (std::chrono::steady_clock::now() - it->second.ts > std::chrono::milliseconds(500)) {
                    if (cb_copy) cb_copy(it->second.path, "DELETED");
                    it = pending_moves.erase(it);
                } else {
                    ++it;
                }
            }
            auto now = std::chrono::steady_clock::now();
            for (auto it = file_events.begin(); it != file_events.end(); ) {
                if (now - it->second.last_time > debounce_window) {
                    if (cb_copy) cb_copy(it->first, "MODIFIED");
                    it = file_events.erase(it);
                } else {
                    ++it;
                }
            }

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
