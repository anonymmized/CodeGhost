#include "./eventprocessor.hpp"
#include "../cli/cli.hpp"
#include "./logger.hpp"
#include "./daemon.hpp"
#include "./hasher.hpp"
#include "./watcher.hpp"
#include "../utils/utils.hpp"
#include "./defaults.hpp"

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <sys/inotify.h>
#include <iostream>

void Processor::prepareConfig() {
    std::error_code ec;
    if (args.configPath.empty()) args.configPath = std::string(DEFAULT_CONFIG_PATH);
    if (!std::filesystem::exists(args.configPath, ec)) {
    	logger->log(LOG_WARN, "Config doesn't exist: " + args.configPath);
	logger->log(LOG_INFO, "Using default config: " + std::string(DEFAULT_CONFIG_PATH));
	args.configPath = std::string(DEFAULT_CONFIG_PATH);
    }
    ec.clear();

    if (!std::filesystem::exists(args.configPath, ec)) {
    	logger->log(LOG_WARN, "Default config wasn't found. Creating new one...");
	std::filesystem::path ppath(args.configPath);
	std::filesystem::path parent = ppath.parent_path();
	std::filesystem::create_directories(parent, ec);
	if (ec) {
	    logger->log(LOG_ERROR, "Failed to create config directory: " + parent.string() + " : " + ec.message());
	    throw std::runtime_error("Failed to create config directory");
	}
	Config new_conf = createDefaultConfig();

    	uploadToConfig(new_conf, std::string(DEFAULT_CONFIG_PATH));
	logger->log(LOG_INFO, "Default config created: " + args.configPath);
    }
}

void Processor::parseArgs() { args = CliParser::parse(argc, argv); }
void Processor::initLogger() {
    std::filesystem::path ppath(args.logPath);
	std::filesystem::path parent = ppath.parent_path();
	std::filesystem::create_directories(parent);
    logger = std::make_unique<Logger>(args.logPath, LOG_INFO, LOG_INFO, true, true);
    logger->log(LOG_INFO, "Logging to: " + args.logPath);
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

void Processor::initWatcher() { watcher = std::make_unique<Watcher>(config); }
void Processor::initHasher() { hasher = std::make_unique<Hasher>(config.ignore_paths, config.watch_recursive); }

void Processor::handleEvent(inotify_event* event) {
    if (!watcher->hasWatch(event->wd)) {
        return;
    }
    std::string name = event->len ? event->name : "";
    std::string full_path = watcher->getFullPath(event->wd, name);
    if (event->mask & IN_CREATE) {
        if (std::filesystem::exists(full_path) && std::filesystem::is_directory(full_path))
            watcher->registerRecursive(full_path);
        hasher->fileChanged(full_path, *logger);
    }
    if (event->mask & IN_MODIFY) {
        hasher->fileChanged(full_path, *logger);
    }
    if (event->mask & IN_DELETE) {
        hasher->deleteHash(full_path, *logger);
    }
    if (event->mask & IN_MOVED_FROM) {
        hasher->fileMoved(full_path, *logger, false, event->cookie);
    }
    if (event->mask & IN_MOVED_TO) {
        hasher->fileMoved(full_path, *logger, true, event->cookie);
    }
    if (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_IGNORED)) {
        std::string removed = watcher->getFullPath(event->wd, "");
        watcher->removeWatcher(event->wd);
        logger->log(LOG_WARN, "Watcher removed: " + removed);
    }
    return;
}

void Processor::run(int _argc, char** _argv) {
    argc = _argc;
    argv = _argv;
    parseArgs();

    if (args.daemonise) daemonise();
    // create and logger and log into marked file
    initLogger();

    prepareConfig();
    // create config with errors handling
    initConfig();
    validateWatchPaths();
    if (config.watch_paths.empty()) {
        logger->log(LOG_ERROR, "No valid watch paths left");
        return;
    }
    // create watcher by config
    initWatcher();
    // create hasher by config's vars
    initHasher();
    if (!std::filesystem::exists(DEFAULT_BASELINE_PATH)) {
    	std::filesystem::create_directories(std::filesystem::path(DEFAULT_BASELINE_PATH).parent_path());
	for (const auto& path : config.watch_paths) {
	    hasher->initHashes(config, path, std::string(DEFAULT_BASELINE_PATH));
	}
    } else hasher->loadBaselineFile(std::string(DEFAULT_BASELINE_PATH));
    logger->log(LOG_INFO, "Hashes were calculated.");
    if (config.watch_recursive) {
        for (const auto& path : config.watch_paths) {
            watcher->registerRecursive(path);
        }
    } else {
	    for (const auto& path : config.watch_paths) {
            if (!shouldIgnoreTree(path, config.ignore_paths))
            	watcher->addWatch(path);
	    }
    }
    char buffer[4096];
    for (const auto& [wd, path] : watcher->getWatchTable()) logger->log(LOG_INFO, "Watching: " + path);
    while (true) {
        int len = read(watcher->getFd(), buffer, sizeof(buffer));
        int i = 0;
        while (i < len) {
            auto* event = reinterpret_cast<inotify_event*>(&buffer[i]);
            handleEvent(event);
            i += sizeof(inotify_event) + event->len;
        }
    }
}
