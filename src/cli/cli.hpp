#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>

struct CliArgs {
  bool daemonise = false;
  std::string configPath;
  std::string logPath;
  std::string partsPath;
};

class CliParser {
public:
  static CliArgs parse(int argc, char* argv[]);
  static void printUsage();
};

std::unordered_map<std::string, std::vector<std::string>> parseICW(const std::string& path);
std::string trimmed(const std::string& str);

