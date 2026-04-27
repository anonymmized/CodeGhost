#pragma once

#include <unordered_map>
#include <optional>
#include <cstdint>
#include <string>
#include <chrono>

class MoveTracker {
    private:
        struct PendingMove {
            std::string path;
            std::chrono::steady_clock::time_point ts;
        };
        std::unordered_map<uint32_t, PendingMove> pending;
        std::chrono::milliseconds move_timeout;
    public:
        MoveTracker(std::chrono::milliseconds mv_timeout) : move_timeout(std::move(mv_timeout)) {}
        std::optional<std::string> onMovedTo(uint32_t cookie, const std::string& new_path);
        void onMovedFrom(uint32_t cookie, const std::string& path);
        template<typename Callback>
            void flush(Callback cb) {
                auto now = std::chrono::steady_clock::now();
                for (auto it = pending.begin(); it != pending.end(); ) {
                    if (now - it->second.ts > move_timeout) {
                        cb(it->second.path, "DELETED");
                        it = pending.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
};
