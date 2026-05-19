#pragma once

#include "../cli/cli.hpp"
#include "./daemon.hpp"
#include "./hasher.hpp"
#include "./logger.hpp"
#include "./watcher.hpp"
#include "../utils/utils.hpp"

#include <memory>
#include <unordered_map>
#include <sys/inotify.h>
#include <filesystem>
#include <chrono>

enum class EventType {
    Create,
    Modify,
    Delete,
    MoveFrom,
    MoveTo
};

struct FsEvent {
    EventType type;
    uint32_t cookie;
    std::chrono::steady_clock::time_point timestamp;
};


class Processor {
    private:
        Config config;
        CliArgs args;
        std::unique_ptr<Logger> logger;
        std::unique_ptr<Watcher> watcher;
        std::unique_ptr<Hasher> hasher;
        std::unordered_map<std::string, std::vector<FsEvent>> pending_events;
        int argc;
        char** argv;
    public:
        void prepareConfig();
        void parseArgs();
        void initLogger();
        void initConfig();
        void initWatcher();
        void initHasher();
        void validateWatchPaths();
        EventType normalizeEvents(const std::vector<FsEvent>& events);
        void processPendingEvents();
        void collectEvent(inotify_event* event);
        void run(int _argc, char** _argv);
};

void handleSig(int);
