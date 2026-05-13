#include "./eventprocessor.hpp"
#include "../cli/cli.hpp"
#include "./logger.hpp"
#include "./daemon.hpp"
#include "./hasher.hpp"
#include "./watcher.hpp"
#include "../utils/utils.hpp"

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <sys/inotify.h>
#include <iostream>

void Processor::parseArgs() { args = CliParser::parse(argc, argv); }
void Processor::uploadAllin() {
    if (!args.partsPath.empty())
        allin = parseICW(args.partsPath);
}
void Processor::initLogger() { 
    std::cout << "LOGPATH[" << args.logPath << "]\n";
    logger = std::make_unique<Logger>(args.logPath, LOG_INFO, LOG_INFO, true, true);
    logger->log(LOG_INFO, "Logging to: " + args.logPath);
    logger->log(LOG_INFO, std::string(argv[0]) + " started.");
}
void Processor::initConfig() {
    try {
        config = loadFromConfig(args.configPath);
        logger->log(LOG_INFO, "Config is loaded. Recursive: " + std::to_string(config.watch_recursive));
        if (config.watch_paths.empty() && config.critical_paths.empty()) {
            logger->log(LOG_WARN, "No paths were provided. Using default ones...");
            config.watch_paths = allin["watch"];
            config.critical_paths = allin["critical"];
            config.ignore_paths = allin["ignore"];
            config.watch_recursive = true;
        }
    } catch (const std::runtime_error& e) {
        logger->log(LOG_ERROR, e.what());
        logger->log(LOG_INFO, "Using default config values.");
    } catch (...) {
        logger->log(LOG_ERROR, "Reading from config failed for no reason.");
        logger->log(LOG_INFO, "Using default config values.");
    }
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
    /*
    if (event->mask & IN_DELETE) {
        watcher->removeWatcher(event->wd);
    }
    */
    return;
}

void Processor::run(int _argc, char** _argv) {
    argc = _argc;
    argv = _argv;
    parseArgs();

    uploadAllin();

    if (args.daemonise) daemonise();
    // create and logger and log into marked file
    initLogger();
    // create config with errors handling
    initConfig();
    // create watcher by config
    initWatcher();
    // create hasher by config's vars
    initHasher();
    hasher->loadBaselineFile("baseline.json");
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
