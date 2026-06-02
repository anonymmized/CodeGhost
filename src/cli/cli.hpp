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
    std::string serverUrl;
    int pollInterval = 30;
    std::string pendingPath;
};

class CliParser {
public:
    static CliArgs parse(int argc, char* argv[]);
    static void printUsage();
};
