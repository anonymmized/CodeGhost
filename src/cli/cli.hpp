#pragma once

#include <string>

struct CliArgs {
    bool daemonise = false;
    bool approveRuntime = false;
    bool reloadRuntime = false;
    std::string configPath;
    std::string logPath;
    std::string loginPath;
};

class CliParser {
public:
    static CliArgs parse(int argc, char* argv[]);
    static void printUsage();
};
