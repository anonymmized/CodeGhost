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
    // фильтрация мусорных файлов, в редакторах кода при создании/записи и тд также создаются временные файлы, чтобы их исключить нужна эта функция
    // тут игнорируются временные файлы и скрытые файлы
    if (name.empty()) return true;
    if (name.starts_with(".#")) return true;
    if (name.ends_with("~") || name.ends_with(".swp") || name.ends_with(".swo") || name.ends_with(".tmp")) return true;
    if (name[0] == '.') return true;
    return false;
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
    // защита от повторного запуска 
    if (running.exchange(true)) return;
    worker = std::thread([this]() {
        // из-за IN_NONBLOCK read не блокирует поток потому что иначе stop зависнет 
        int fd = inotify_init1(IN_NONBLOCK);
        if (fd == -1) {
            std::cerr << "Failed to initialize inotify: " << strerror(errno) << '\n';
            running = false;
            return;
        }
        // что будет детектить IN_CLOSE_WRITE эьто сохранение 
        int wd = inotify_add_watch(fd, watch_path.c_str(), IN_CLOSE_WRITE);
        if (wd == -1) {
            std::cerr << "Failed to add watch: " << strerror(errno) << '\n';
            close(fd);
            running = false;
            return;
        }
        // буфер для чтения событий из ядра alignas нужен для корректного приведения к inotify_event*
        alignas(inotify_event) char buf[4096];
        // debounce - подавление дублирующих событий
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_event_time;
        const auto debounce_window = std::chrono::milliseconds(150);
        while (running) {
            // чтения событий из ядра
            ssize_t len = read(fd, buf, sizeof(buf));
            if (len < 0) {
                if (errno == EAGAIN) {
                    // если нет событий, то слип
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
                if (errno == EINTR) continue;
                std::cerr << "read error: " << strerror(errno) << '\n';
                break;
            }
            ssize_t i = 0;
            // в буфере может быть несколько событий подряд
            while (i < len) {
                auto* event = reinterpret_cast<inotify_event*>(buf + i);
                if (event->len > 0) {
                    std::string filename = event->name;
                    if (shouldIgnoreFile(filename)) {
                        i += sizeof(inotify_event) + event->len;
                        continue;
                    }
                    // формирование полного пути чтобы дальше использовать 
                    std::string full_path = (std::filesystem::path(watch_path) / filename).string();
                    // игнор всего кроме сохранения файла
                    if (!(event->mask & IN_CLOSE_WRITE)) {
                        i += sizeof(inotify_event) + event->len;
                        continue;
                    }
                    // дебаунс - фильтруем частые одинаковые события
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
                        // копирование callback под лок чтобы не держать mutex при вызове 
                        std::lock_guard<std::mutex> lock(callback_mutex);
                        cb_copy = callback;
                    }

                    if (cb_copy) {
                        // вызывается пользовательская логика
                        cb_copy(full_path, "MODIFIED");
                    }
                    last_event_time[filename] = now;
                }
                // переход к некст событию
                i += sizeof(inotify_event) + event->len;
            }
        }
        // очистка ресурсов ядра
        inotify_rm_watch(fd, wd);
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
