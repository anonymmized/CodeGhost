#include "cli.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <cctype>
#include <algorithm>

std::string trimmed(const std::string& str) {
    auto first = std::find_if_not(str.begin(), str.end(),
                                  [](unsigned char c){ return std::isspace(c); });
    auto last = std::find_if_not(str.rbegin(), str.rend(),
                                  [](unsigned char c){ return std::isspace(c); }).base();
    if (first >= last) return {};
    return std::string(first, last);
}

std::unordered_map<std::string, std::vector<std::string>> parseICW(const std::string& path) {
  std::ifstream infile(path);
  if (!infile.is_open())
    throw std::runtime_error("File wasn't opened");
  std::string line;
  int state = -1; // 0 - IGNORE 1 - CRIT 2 - WATCH
  std::unordered_map<std::string, std::vector<std::string>> allin;
  std::vector<std::string> to_ignore;
  std::vector<std::string> critical;
  std::vector<std::string> to_watch;
  while (std::getline(infile, line)) {
    line = trimmed(line);
    if (line == "[IGNORE]") {
      state = 0;
      continue;
    } else if (line == "[CRIT]") {
      state = 1;
      continue;
    } else if (line == "[WATCH]") {
      state = 2;
      continue;
    } else {
      if (line.empty() || line[0] == '#') continue;
      if (state == -1) {
        continue;
      } else if (state == 0) {
        to_ignore.push_back(line);
      } else if (state == 1) {
        critical.push_back(line);
      } else if (state == 2) {
        to_watch.push_back(line);
      }
    }
  }
  infile.close();
  allin["ignore"] = to_ignore;
  allin["critical"] = critical;
  allin["watch"] = to_watch;
  return allin;
}

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
    } else if (arg.rfind("--rules=", 0) == 0) {
      args.partsPath = arg.substr(8);
    } else {
      std::cerr << "Unknown argument" << arg << std::endl;
      printUsage();
      exit(1);
    }
    if (args.configPath.empty()) {
      const char* env = std::getenv("CONFIG_PATH_SEC_ANALYZER");
      if (env) 
        args.configPath = env;
    }
    if (args.logPath.empty())
      args.logPath = "daemon.log";
    if (args.configPath.empty())
      args.configPath = "config.json";
  }
  return args;
}

void CliParser::printUsage() {
  // Repair the name of the program
  std::cout << "Usage: security-analyzer" << " --config=<path> [--daemonise]\n"
    << " --config=<path>  path to config file json\n"
    << " --daemonise      run as background daemon\n"
    << " --log=<path>     path to log file (dafault: daemon.log)"
    << " --rules=<path>   path to file includes ignored|critical|watching files list";
}


