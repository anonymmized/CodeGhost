#pragma once
#include <string>
#include <functional>

class Watcher {
    public:
        explicit Watcher(std::string path_to_watch);
        //~Watcher();
        void start();
        void stop();
        using EventCallback = std::function<void(const std::string& file_path, const std::string& event_type)>;
        void setCallback(EventCallback cb);
    private:
        std::string watch_path;
        int inotify_id = -1;
        bool running = false;
        EventCallback callback;
};
