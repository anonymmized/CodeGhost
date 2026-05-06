#include "./cli/cli.hpp"
#include "./core/daemon.cpp"

int main(int arc, char* argv[]) {
  CLiArgs args = CliParser::parse(argc, argv);
  
  if (args.daemonize) daemonise();
  loadFromConfig(argv.configPath);

  return 0;
}
