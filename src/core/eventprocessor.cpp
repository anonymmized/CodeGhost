#include "./eventprocessor.hpp"

#include "./runtime_constants.hpp"

#include <atomic>
#include <cerrno>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <poll.h>
#include <unistd.h>

inline std::atomic<bool> running(true);

void Processor::prepareConfig() {
    std::error_code ec;
    if (args.configPath.empty()) {
        args.configPath = std::string(runtime::DEFAULT_CONFIG_PATH);
    }

    if (!std::filesystem::exists(args.configPath, ec)) {
        logger->log(LOG_WARN, "Config doesn't exist: " + args.configPath);
        logger->log(LOG_INFO, "Using default config: " + std::string(runtime::DEFAULT_CONFIG_PATH));
        args.configPath = std::string(runtime::DEFAULT_CONFIG_PATH);
    }

    ec.clear();
    if (!std::filesystem::exists(args.configPath, ec)) {
        logger->log(LOG_WARN, "Default config wasn't found. Creating a new one.");
        std::filesystem::path config_path(args.configPath);
        std::filesystem::path parent = config_path.parent_path();
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            logger->log(LOG_ERROR, "Failed to create config directory: " + parent.string() + " : " + ec.message());
            throw std::runtime_error("Failed to create config directory");
        }

        Config new_config = createDefaultConfig();
        uploadToConfig(new_config, std::string(runtime::DEFAULT_CONFIG_PATH));
        logger->log(LOG_INFO, "Default config created: " + args.configPath);
    }
}

void Processor::parseArgs() {
    args = CliParser::parse(argc, argv);
}

void Processor::initLogger() {
    std::filesystem::path log_path(args.logPath);
    std::filesystem::path parent = log_path.parent_path();
    std::filesystem::create_directories(parent);

    logger = std::make_unique<Logger>(
        args.logPath,
        LOG_INFO,
        LOG_INFO,
        true,
        true,
        !args.serverIp.empty(),
        args.serverIp.empty() ? args.serverIp : "",
	args.serverPort.empty() ? args.serverPort : "10101",
    );
    logger->log(LOG_INFO, "Logging to: " + args.logPath);
    if (!args.serverIp.empty()) {
        logger->log(LOG_INFO, "Trying to connect to server: " + args.serverIp );
    }
    logger->log(LOG_INFO, std::string(argv[0]) + " started.");
}

void Processor::initConfig() {
    config = loadFromConfig(args.configPath);
    logger->log(LOG_INFO, "Config loaded: " + args.configPath);
    logger->log(LOG_INFO, "Recursive mode: " + std::to_string(config.watch_recursive));
}

void Processor::validateWatchPaths() {
    std::vector<std::string> valid_paths;
    for (const auto& path : config.watch_paths) {
        if (!std::filesystem::exists(path)) {
            logger->log(LOG_ERROR, "Watch path doesn't exist: " + path);
            continue;
        }
        if (!std::filesystem::is_directory(path)) {
            logger->log(LOG_ERROR, "Watch path is not a directory: " + path);
            continue;
        }
        try {
            std::filesystem::directory_iterator it(path);
        } catch (const std::filesystem::filesystem_error& e) {
            logger->log(LOG_ERROR, "No access to watch path: " + path + " : " + e.what());
            continue;
        }
        valid_paths.push_back(path);
    }
    config.watch_paths = std::move(valid_paths);
}

void Processor::initWatcher() {
    watcher = std::make_unique<Watcher>(config);
}

void Processor::initHasher() {
    hasher = std::make_unique<Hasher>(config.ignore_paths, config.critical_paths, config.watch_recursive);
}

void Processor::applyRuntimeFlags() {
    if (args.reloadRuntime) {
        hasher->reloadRuntime(config);
        logger->log(LOG_INFO, "Runtime state rebuilt from filesystem. Baseline was not changed.");
    }

    if (args.approveRuntime) {
        hasher->syncBaseline(std::string(runtime::DEFAULT_BASELINE_PATH));
        logger->log(LOG_INFO, "Runtime state approved and persisted as new baseline.");
    }
}

void Processor::processMoveTo(const std::string& new_path, uint32_t cookie, bool is_dir) {
    auto pending_move_it = pending_moves.find(cookie);
    if (pending_move_it == pending_moves.end()) {
        if (is_dir) {
            if (!shouldIgnoreTree(new_path, config.ignore_paths)) {
                watcher->registerRecursive(new_path);
                hasher->registerPathTree(new_path, *logger);
            }
        } else {
            hasher->fileChanged(new_path, *logger);
        }
        return;
    }

    PendingMove pending_move = pending_move_it->second;
    pending_moves.erase(pending_move_it);

    if (pending_move.is_dir) {
        watcher->renameSubtree(pending_move.old_path, new_path);
        if (shouldIgnoreTree(new_path, config.ignore_paths)) {
            watcher->removeSubtree(new_path);
            hasher->deletePathTree(pending_move.old_path, *logger);
            logger->log(LOG_INFO, "Moved directory into ignored path: " + pending_move.old_path + " -> " + new_path);
            return;
        }
        watcher->registerRecursive(new_path);
    }

    hasher->movePathTree(pending_move.old_path, new_path, *logger, pending_move.is_dir);
}

void Processor::processExpiredMoves() {
    const auto now = std::chrono::steady_clock::now();
    for (auto it = pending_moves.begin(); it != pending_moves.end();) {
        if (now - it->second.timestamp < runtime::EVENT_DEBOUNCE) {
            ++it;
            continue;
        }

        if (it->second.is_dir) {
            watcher->removeSubtree(it->second.old_path);
            hasher->deletePathTree(it->second.old_path, *logger);
        } else {
            hasher->deleteHash(it->second.old_path, *logger);
        }

        it = pending_moves.erase(it);
    }
}

void Processor::collectEvent(inotify_event* event) {
    if (!watcher->hasWatch(event->wd)) return;

    std::string name = event->len ? event->name : "";
    std::string full_path = watcher->getFullPath(event->wd, name);
    auto now = std::chrono::steady_clock::now();
    bool is_dir = (event->mask & IN_ISDIR) != 0;

    if (event->mask & IN_MOVED_FROM) {
        pending_moves[event->cookie] = PendingMove{full_path, now, is_dir};
        return;
    }

    if (event->mask & IN_MOVED_TO) {
        processMoveTo(full_path, event->cookie, is_dir);
        return;
    }

    if (event->mask & IN_CREATE) {
        pending_events[full_path].push_back({EventType::Create, event->cookie, now, is_dir});
    }
    if (event->mask & IN_MODIFY) {
        pending_events[full_path].push_back({EventType::Modify, event->cookie, now, is_dir});
    }
    if (event->mask & IN_DELETE) {
        pending_events[full_path].push_back({EventType::Delete, event->cookie, now, is_dir});
    }
    if (event->mask & IN_ATTRIB) {
        pending_events[full_path].push_back({EventType::Attrib, event->cookie, now, is_dir});
    }
}

EventType Processor::normalizeEvents(const std::vector<FsEvent>& events) {
    bool has_create = false;
    bool has_modify = false;
    bool has_delete = false;
    bool has_attrib = false;

    for (const auto& event : events) {
        switch (event.type) {
            case EventType::Create:
                has_create = true;
                break;
            case EventType::Modify:
                has_modify = true;
                break;
            case EventType::Delete:
                has_delete = true;
                break;
            case EventType::Attrib:
                has_attrib = true;
                break;
            default:
                break;
        }
    }

    if (has_delete) return EventType::Delete;
    if (has_create) return EventType::Create;
    if (has_modify) return EventType::Modify;
    if (has_attrib) return EventType::Attrib;
    return events.back().type;
}

void Processor::processPendingEvents() {
    auto now = std::chrono::steady_clock::now();

    for (auto it = pending_events.begin(); it != pending_events.end();) {
        const std::string& path = it->first;
        const std::vector<FsEvent>& events = it->second;

        if (events.empty()) {
            it = pending_events.erase(it);
            continue;
        }

        const auto& last_event = events.back();
        if (now - last_event.timestamp < runtime::EVENT_DEBOUNCE) {
            ++it;
            continue;
        }

        EventType type = normalizeEvents(events);
        bool is_dir = last_event.is_dir;
        switch (type) {
            case EventType::Modify:
                try {
                    hasher->fileChanged(path, *logger);
                } catch (const std::exception& e) {
                    logger->log(LOG_WARN, "Failed to process modified path: " + path + " : " + e.what());
                }
                break;
            case EventType::Delete:
                try {
                    if (is_dir) {
                        watcher->removeSubtree(path);
                        hasher->deletePathTree(path, *logger);
                    } else {
                        hasher->deleteHash(path, *logger);
                    }
                } catch (const std::exception& e) {
                    logger->log(LOG_WARN, "Failed to process deleted path: " + path + " : " + e.what());
                }
                break;
            case EventType::Create:
                try {
                    if (is_dir) {
                        watcher->registerRecursive(path);
                        hasher->registerPathTree(path, *logger);
                    } else {
                        hasher->fileChanged(path, *logger);
                    }
                } catch (const std::exception& e) {
                    logger->log(LOG_WARN, "Failed to process created path: " + path + " : " + e.what());
                }
                break;
            case EventType::Attrib:
                try {
                    if (!is_dir) {
                        hasher->fileAttributed(path, *logger);
                    }
                } catch (const std::exception& e) {
                    logger->log(LOG_WARN, "Failed to process metadata change: " + path + " : " + e.what());
                }
                break;
            default:
                break;
        }

        it = pending_events.erase(it);
    }
}

void handleSig(int) {
    running.store(false);
}

void Processor::run(int _argc, char** _argv) {
    struct sigaction sa {};
    sa.sa_handler = handleSig;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    argc = _argc;
    argv = _argv;
    parseArgs();

    if (args.daemonise) daemonise();
    initLogger();
    prepareConfig();
    initConfig();
    validateWatchPaths();
    if (config.watch_paths.empty()) {
        logger->log(LOG_ERROR, "No valid watch paths left");
        return;
    }

    initWatcher();
    initHasher();

    if (!std::filesystem::exists(runtime::DEFAULT_BASELINE_PATH)) {
        std::filesystem::create_directories(std::filesystem::path(runtime::DEFAULT_BASELINE_PATH).parent_path());
        hasher->initHashes(config);
        hasher->saveBaseline(std::string(runtime::DEFAULT_BASELINE_PATH));
        logger->log(LOG_INFO, "Baseline created: " + std::string(runtime::DEFAULT_BASELINE_PATH));
    } else {
        hasher->loadBaselineFile(std::string(runtime::DEFAULT_BASELINE_PATH));
    }
    logger->log(LOG_INFO, "Baseline initialized.");
    applyRuntimeFlags();

    if (config.watch_recursive) {
        for (const auto& path : config.watch_paths) {
            watcher->registerRecursive(path);
        }
    } else {
        for (const auto& path : config.watch_paths) {
            if (!shouldIgnoreTree(path, config.ignore_paths)) {
                watcher->addWatch(path);
            }
        }
    }

    char buffer[runtime::INOTIFY_BUFFER_SIZE];
    for (const auto& [wd, path] : watcher->getWatchTable()) {
        logger->log(LOG_INFO, "Watching: " + path);
    }

    while (running.load()) {
        pollfd pfd{watcher->getFd(), POLLIN, 0};
        int ready = poll(&pfd, 1, runtime::POLL_TIMEOUT_MS);
        if (ready < 0) {
            if (errno == EINTR) {
                if (!running.load()) break;
                continue;
            }
            logger->log(LOG_ERROR, "poll() failed");
            break;
        }

        if (ready == 0) {
            processExpiredMoves();
            processPendingEvents();
            continue;
        }

        int len = read(watcher->getFd(), buffer, sizeof(buffer));
        if (len < 0) {
            if (errno == EINTR) {
                if (!running.load()) break;
                continue;
            }
            logger->log(LOG_ERROR, "read() failed");
            break;
        }
        if (len == 0) continue;

        int offset = 0;
        while (offset < len) {
            auto* event = reinterpret_cast<inotify_event*>(&buffer[offset]);
            collectEvent(event);
            offset += sizeof(inotify_event) + event->len;
        }

        processExpiredMoves();
        processPendingEvents();
    }

    logger->log(LOG_INFO, "Daemon stopped");
}
