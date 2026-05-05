#pragma once
#include <string>

struct CliArgs {
  bool daemonise = false;
  std::string configPath;
};

class CliParser {
public:
  static CliArgs parse(int argc, char* argv[]);
  static void printUsage();
};
