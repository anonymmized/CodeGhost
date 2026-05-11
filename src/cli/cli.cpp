#include "cli.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>

CliArgs CliParser::parse(int argc, char* argv[]) {
  CliArgs args;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--daemonise") {
       args.daemonise = true;
    } else if (arg.rfind("--config", 0) == 0) {
      args.configPath = arg.substr(9);
    } else if (arg.rfind("--log", 0) == 0) {
      args.logPath = arg.substr(6);
    } else if (arg.rfind("--aspects", 0) == 0) {
      args.ignorePath = args.substr(10);
    } else {
      std::cerr << "Unknown argument" << arg << std::endl;
      printUsage();
      exit(1);
    }
    if (args.configPath.empty()) {
      args.configPath = std::getenv("CONFIG_PATH_SEC_ANALYZER");
    }
    if (args.logPath.empty())
      args.logPath("daemon.log");
  }
  return args;
}

void CliParser::printUsage() {
  // Repair the name of the program
  std::cout << "Usage: security-analyzer" << " --config=<path> [--daemonise]\n"
    << " --config=<path>  path to config file json\n"
    << " --daemonise      run as background daemon\n"
    << " --log=<path>     path to log file (dafault: daemon.log)"
    << " --ignore=<path>  path to file includes ignored files list";
}


