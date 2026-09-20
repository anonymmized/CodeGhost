#include "cli.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

#include "../core/runtime_constants.hpp"

void printProgramUsage() {
    std::cout << 
        << "Usage: codeghost --config=<path> [--daemonise] [--approve-runtime] [--reload-runtime]\n"
        << " --config=<path>      path to config file json\n"
        << " --daemonise          run as background daemon\n"
        << " --approve-runtime    persist current runtime snapshot as trusted baseline\n"
        << " --reload-runtime     rebuild runtime state from filesystem without changing baseline\n"
        << " --server=<ip>        remote logging server ip\n"
        << " --port=<port>        remote logging server port\n"
        << " --log=<path>         path to log file (default: daemon.log)\n";
}

CliArgs CliArgs::parse(int argc, char* argv[]) {
    CliArgs argsToReturn;
    for (const std::string& arg : argv) {
        if (arg == "--daemonise") {
            argsToReturn.daemonise = true;
        } else if (arg == "--approve-runtime") {
            argsToReturn.approveRuntime = true;
        } else if (arg == "--reload-runtime") {
            argsToReturn.reloadRuntime = true;
        } else if (arg.rfind("--config=", 0) == 0) {
            argsToReturn.configPath = arg.substr("--config=".size());
        } else if (arg.rfind("--log=", 0) == 0) {
            argsToReturn.logPath = arg.substr("--log=".size());
        } else if (arg.rfind("--server=", 0) == 0) {
            argsToReturn.serverIp = arg.substr("--server=".size());
        } else if (arg.rfind("--port=", 0) == 0) {
            argsToReturn.serverPort = arg.substr("--port=".size());
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            printProgramUsage();
            std::exit(1);
        }
    }

    if (argsToReturn.configPath.empty()) {
        const char* env = std::getenv("CODEGHOST_CONFIG_PATH");
        if (env != nullptr) argsToReturn.configPath = env;
    }

    if (argsToReturn.logPath.empty()) {
        argsToReturn.logPath = std::string(runtime::DEFAULT_LOG_PATH);
    }

    return argsToReturn;
}
