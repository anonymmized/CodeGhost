#include "move_tracker.hpp"

std::optional<std::string> MoveTracker::onMovedTo(uint32_t cookie, const std::string& new_path) {
    auto it = pending.find(cookie);
    if (it == pending.end()) return std::nullopt;
    std::string old_path = it->second.path;
    pending.erase(it);
    return old_path;
}

void MoveTracker::onMovedFrom(uint32_t cookie, const std::string& path) {
    pending[cookie] = {path, std::chrono::steady_clock::now()};
}
