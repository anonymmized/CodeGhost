#include "watcher.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/inotify.h>
#include <cstring>

Watcher::Watcher(std::string path_to_watch) : watch_path(std::move(path_to_watch)) {}
//Watcher::~Watcher() { stop(); }
void Watcher::setCallback(EventCallback cb) { callback = std::move(cb); }

void Watcher::start() {
    std::cout << "Watcher starting for path: " << watch_path << '\n';
    inotify_id = inotify_init();
    if (inotify_id == -1) {
        std::cerr << "Failed to initialize inotify: " << strerror(errno) << '\n';
        return;
    }
    int wd = inotify_add_watch(inotify_id, watch_path.c_str(), IN_CLOSE_WRITE | IN_CREATE | IN_DELETE);
    char buf[4096];
    running = true;
    while (running) {
        ssize_t len = read(inotify_id, buf, sizeof(buf));
        if (len <= 0) {
            perror("read");
            continue;
        }
        ssize_t i = 0;
        while (i < len) {
            auto* event = reinterpret_cast<inotify_event*>(&buf[i]);
            if (event->len > 0) {
                std::string file = event->name;
                std::string type;
                if (event->mask & IN_CLOSE_WRITE) type = "WRITE";
                else if (event->mask & IN_MODIFY) type = "MODIFY";
                else if (event->mask & IN_CREATE) type = "CREATE";
                else if (event->mask & IN_DELETE) type = "DELETE";
                else if (event->mask & IN_MOVED_TO) type = "MOVE";
                if (!type.empty()) {
                    std::cout << "|Watcher| " << type << ": " << file << '\n';
                    if (callback) callback(file, type);
                }
            }
            i += sizeof(inotify_event) + event->len;
        }
    }
    inotify_rm_watch(inotify_id, wd);
    close(inotify_id);
}

int main() {
    Watcher watchr("./");
    watchr.setCallback([](const std::string& path, const std::string& type) { std::cout << "|Callback| " << type << " -> " << path << '\n'; });
    watchr.start();
    return 0;
}
