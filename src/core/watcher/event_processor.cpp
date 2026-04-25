#include "event_processor.hpp"
#include "watch_registry.hpp"
#include "move_tracker.hpp"
#include "debounce_buffer.hpp"
#include "utils.hpp"
#include <filesystem>

namespace fs = std::filesystem;

EventProcessor::EventProcessor(WatchRegistry& wr, MoveTracker& mt, DebounceBuffer& db, Callback cb) : wr(wr), mt(mt), db(db), cb(std::move(cb)) {}

void EventProcessor::setCallback(Callback new_cb) { cb = std::move(new_cb); }

void EventProcessor::process(const inotify_event* event, const std::string& base_path) {
    if (base_path.empty()) return;
    if (event->mask & IN_IGNORED) {
        wr.remove(event->wd);
        return;
    }
    if (event->mask & IN_DELETE_SELF) {
        wr.removeSubtree(base_path, event->wd);
        if (cb) cb(base_path, "DELETED");
        return;
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
        return;
    }
    if (event->len > 0) {
        std::string filename = event->name;
        if (shouldIgnoreFile(filename)) return;
        std::string full_path = base_path + "/" + filename;
        if (event->mask & IN_MOVED_FROM) {
            mt.onMovedFrom(event->cookie, full_path);
            return;
        }
        if (event->mask & IN_MOVED_TO) {
            if (auto old_path = mt.onMovedTo(event->cookie, full_path)) {
                if (cb) cb(*old_path + "|" + full_path, "RENAMED");
            } else {
                db.touch(full_path);
            }
            return;
        }
        if (event->mask & IN_DELETE) {
            if (cb) cb(full_path, "DELETED");
            return;
        }
        if (!(event->mask & IN_CLOSE_WRITE)) return;
        db.touch(full_path);
    }
}

void EventProcessor::flush() {
    if (!cb) return;
    mt.flush(cb);
    db.flush(cb);
}
