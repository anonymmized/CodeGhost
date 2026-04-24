#pragma once

#include <unordered_map>
#include <string>
#include <chrono>

class DebounceBuffer {
    private:
        struct State { std::chrono::steady_clock::time_point ts; };
        std::unordered_map<std::string, State> files;
    public:
        void touch(const std::string& path);
        template<typename Callback>
        void flush(Callback cb) {
            auto now = std::chrono::steady_clock::now();
            const auto debounce_window = std::chrono::milliseconds(150);
            for (auto it = files.begin(); it != files.end(); ) {
                if (now - it->second.ts > debounce_window) {
                    cb(it->first, "MODIFIED");
                    it = files.erase(it);
                } else {
                    ++it;
                }
            }
        }
};
