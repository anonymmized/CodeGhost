#pragma once

#include "../cli/cli.hpp"
#include "../utils/utils.hpp"
#include "./daemon.hpp"
#include "./hasher.hpp"
#include "./logger.hpp"
#include "./watcher.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class EventType {
    Create,
    Modify,
    Delete,
    Attrib,
    MoveFrom,
    MoveTo
};

struct FsEvent {
    EventType type;
    uint32_t cookie;
    std::chrono::steady_clock::time_point timestamp;
    bool is_dir = false;
};

struct PendingMove {
    std::string old_path;
    std::chrono::steady_clock::time_point timestamp;
    bool is_dir = false;
};

class Processor {
private:
    Config config;
    CliArgs args;
    std::unique_ptr<Logger> logger;
    std::unique_ptr<Watcher> watcher;
    std::unique_ptr<Hasher> hasher;
    std::unordered_map<std::string, std::vector<FsEvent>> pending_events;
    std::unordered_map<uint32_t, PendingMove> pending_moves;
    int argc = 0;
    char** argv = nullptr;

    void processMoveTo(const std::string& new_path, uint32_t cookie, bool is_dir);
    void processExpiredMoves();
    void applyRuntimeFlags();

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
