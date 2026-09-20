#pragma once

#include <string>

struct CliArgs {
    bool daemonise = false;
    bool approveRuntime = false;
    bool reloadRuntime = false;
    std::string configPath;
    std::string logPath;
    std::string serverIp;
    std::string serverPort;
    CliArgs parse(int argc, char* argv[]);
};
