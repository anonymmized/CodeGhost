#include "cli.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

#include "../core/runtime_constants.hpp"

CliArgs CliParser::parse(int argc, char* argv[]) {
    CliArgs args;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--daemonise") {
            args.daemonise = true;
        } else if (arg == "--approve-runtime") {
            args.approveRuntime = true;
        } else if (arg == "--reload-runtime") {
            args.reloadRuntime = true;
        } else if (arg.rfind("--config=", 0) == 0) {
            args.configPath = arg.substr(9);
        } else if (arg.rfind("--log=", 0) == 0) {
            args.logPath = arg.substr(6);
        } else if (arg.rfind("--login=", 0) == 0) {
            args.loginPath = arg.substr(8);
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            printUsage();
            std::exit(1);
        }
    }

    if (args.configPath.empty()) {
        const char* env = std::getenv("CODEGHOST_CONFIG_PATH");
        if (env != nullptr) args.configPath = env;
    }

    if (args.logPath.empty()) {
        args.logPath = std::string(runtime::DEFAULT_LOG_PATH);
    }

    return args;
}

void CliParser::printUsage() {
    std::cout
        << "Usage: codeghost --config=<path> [--daemonise] [--approve-runtime] [--reload-runtime]\n"
        << " --config=<path>      path to config file json\n"
        << " --daemonise          run as background daemon\n"
        << " --approve-runtime    persist current runtime snapshot as trusted baseline\n"
        << " --reload-runtime     rebuild runtime state from filesystem without changing baseline\n"
        << " --login=<path>       path to server login credentials file\n"
        << " --log=<path>         path to log file (default: daemon.log)\n";
}
