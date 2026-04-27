#pragma once

#include <unordered_map>
#include <string>
#include <chrono>

class DebounceBuffer {
    private:
        struct State { std::chrono::steady_clock::time_point ts; };
        std::unordered_map<std::string, State> files;
        std::chrono::milliseconds window;
    public:
        DebounceBuffer(std::chrono::milliseconds windw) : window(std::move(windw)) {}
        void touch(const std::string& path);
        template<typename Callback>
        void flush(Callback cb) {
            auto now = std::chrono::steady_clock::now();
            for (auto it = files.begin(); it != files.end(); ) {
                if (now - it->second.ts > window) {
                    cb(it->first, "MODIFIED");
                    it = files.erase(it);
                } else {
                    ++it;
                }
            }
        }
};
