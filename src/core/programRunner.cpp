#include "programRunner.hpp"
#include "core/daemon.hpp"

inline std::atomic<bool> isRunningProgram(true);

namespace {

    struct Components {
        Config config;
        std::unique_ptr<Logger> logger;
        std::unique_ptr<Watcher> watcher;
        std::unique_ptr<Hasher> hasher;
    };

    void handleSignal(int signal) {
        isRunningProgram.store(false);
    }

    void initSignals() {
        struct sigaction sigactionStructure {};
        sigactionStructure.sa_handler = handleSignal;
        sigemptyset(&sigactionStructure.sa_mask);
        sigactionStructure.sa_flags = 0;
        sigaction(SIGINT, &sigactionStructure, nullptr);
        sigaction(SIGTERM, &sigactionStructure, nullptr);
    }

    void checkConfigState();

    Logger initLogger();
    Config initConfig() {
        checkConfigState();
    }

    Components prepareComponents() {
        Components components;
        components.logger = initLogger();
        components.config = initConfig();
    }
}

void Runner::run(int argc, char* argv[]) {
    initSignals();
    parsedArguments = parseArguments(argc, argv);
    if (parsedArguments.daemonise) {
        daemoniseProcess();
    }
    prepareComponents();
}
