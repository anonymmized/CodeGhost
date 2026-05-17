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

enum class EventType {
    Create,
    Modify,
    Delete,
    MoveFrom,
    MoveTo
};

struct FsEvent {
    std::string path;
    EventType type;
    uint32_t cookie;
};

class Processor {
    private:
        Config config;
        CliArgs args;
        std::unique_ptr<Logger> logger;
        std::unique_ptr<Watcher> watcher;
        std::unique_ptr<Hasher> hasher;
        std::vector<FsEvent> pending_events;
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
        void processPendingEvents();
        void collectEvent(inotify_event* event);
        void run(int _argc, char** _argv);
};

void handleSig(int);
