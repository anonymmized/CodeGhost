#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>

struct CliArgs {
  bool daemonise = false;
  std::string configPath;
  std::string logPath;
  std::string ignorePath;
};

class CliParser {
public:
  static CliArgs parse(int argc, char* argv[]);
  static void printUsage();
};

std::unordered_map<std::string, std::vector<std::string>> parseICW(const std::string& path) {
  std::ifstream infile(path);
  if (!infile.is_open())
    throw std::runtime_error("File wasn't opened");
  std::string line;
  int state; // 0 - [IGNORE] 1 - [CRIT] 2 - [WATCH]
  std::unordered_map<std::string, std::vector<std::string>> allin;
  std::vector<std::string> to_ignore;
  std::vector<std::string> critical;
  std::vector<std::string> to_watch;
  while (std::getline(infile, line)) {
    if (line == "[IGNORE]") {
      state = 0;
      continue;
    } else if (line == "[CRIT]") {
      state = 1;
      continue;
    } else if (line == "[WATCH]") {
      state = 2;
      continue;
    }
    if (state == 0) {
      to_ignore.push_back(line);
    } else if (state == 1) {
      critical.push_back(line);
    } else if (state == 2) {
      to_watch.push_back(line);
    }
  }
  allin["ignore"] = to_ignore;
  allin["critical"] = critical;
  allin["watch"] = to_watch;
  infile.close();
  return allin;
}


