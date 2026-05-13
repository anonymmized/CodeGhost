#include "cli.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <cctype>
#include <algorithm>

#include "../core/defaults.hpp"

CliArgs CliParser::parse(int argc, char* argv[]) {
  CliArgs args;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--daemonise") {
       args.daemonise = true;
    } else if (arg.rfind("--config=", 0) == 0) {
      args.configPath = arg.substr(9);
    } else if (arg.rfind("--log=", 0) == 0) {
      args.logPath = arg.substr(6);
    } else {
      std::cerr << "Unknown argument" << arg << std::endl;
      printUsage();
      exit(1);
    }
  }
  if (args.configPath.empty()) {
      const char* env = std::getenv("CONFIG_PATH_SEC_ANALYZER");
      if (env) args.configPath = env;
  }
  if (args.logPath.empty()) args.logPath = DEFAULT_LOG_PATH;
  return args;
}

void CliParser::printUsage() {
  // Repair the name of the program
  std::cout << "Usage: security-analyzer" << " --config=<path> [--daemonise]\n"
    << " --config=<path>  path to config file json\n"
    << " --daemonise      run as background daemon\n"
    << " --log=<path>     path to log file (dafault: daemon.log)";
}


