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

class Processor {
    private:
        Config config;
        CliArgs args;
        std::unique_ptr<Logger> logger;
        std::unique_ptr<Watcher> watcher;
        std::unique_ptr<Hasher> hasher;
        int argc;
        char** argv;
    public:
        void prepareConfig();
	void parseArgs();
        void initLogger();
        void initConfig();
        void initWatcher();
        void initHasher();
        void handleEvent(inotify_event* event);
        void validateWatchPaths();
        void run(int _argc, char** _argv);
};
