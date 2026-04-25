#pragma once

#include <functional>
#include <string>
#include <sys/inotify.h>

#include "watch_registry.hpp"
#include "move_tracker.hpp"
#include "debounce_buffer.hpp"

class EventProcessor {
    private:
        WatchRegistry& wr;
        MoveTracker& mt;
        DebounceBuffer& db;
        Callback cb;
    public:
        using Callback = std::function<void(const std::string&, const std::string&)>;
        void setCallback(Callback new_cb);
        EventProcessor(WatchRegistry& wr, MoveTracker& mt, DebounceBuffer& db, Callback cb);
        void process(const inotify_event* event, const std::string& base_path);
        void flush();
};
