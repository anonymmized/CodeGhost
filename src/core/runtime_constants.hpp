#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include "./inotify_compat.hpp"

namespace runtime {
    inline constexpr std::string_view DEFAULT_CONFIG_PATH = "/etc/codeghost/config.json";
    inline constexpr std::string_view DEFAULT_LOG_PATH = "/var/log/codeghost/daemon.log";
    inline constexpr std::string_view DEFAULT_BASELINE_PATH = "/var/lib/codeghost/baseline.json";
    inline constexpr std::string_view DEFAULT_PENDING_PATH = "/var/lib/codeghost/pending.json";

    inline constexpr std::string_view BLUE   = "\x1b[94m";
    inline constexpr std::string_view YELLOW = "\x1b[33m";
    inline constexpr std::string_view RED    = "\x1b[31m";
    inline constexpr std::string_view CLR    = "\x1b[0m";

    inline constexpr auto EVENT_DEBOUNCE = std::chrono::milliseconds(200);
    inline constexpr int POLL_TIMEOUT_MS = 50;
    inline constexpr std::size_t INOTIFY_BUFFER_SIZE = 4096;
    inline constexpr int DEFAULT_POLL_INTERVAL = 30;

    inline constexpr uint32_t WATCH_MASK = IN_CREATE | IN_DELETE |
                                           IN_MODIFY | IN_MOVED_FROM |
                                           IN_MOVED_TO | IN_ATTRIB |
                                           IN_DELETE_SELF | IN_MOVE_SELF |
                                           IN_IGNORED;
}
