#include "./cli/cli.hpp"
#include "./core/daemon.hpp"
#include "./core/hasher.hpp"
#include "./core/logger.hpp"

int main(int argc, char* argv[]) {
  CliArgs args = CliParser::parse(argc, argv);

  if (args.daemonize) daemonise();
  Logger logger(args.logPath, LOG_INFO, LOG_INFO, true, true);
  logger.log(LOG_INFO, "Logging to: " + args.logPath);
  logger.log(LOG_INFO, std::string(argv[0]) + " started.");
  try {
    config = loadFromConfig(args.configPath);
    logger.log(LOG_INFO, "Config is loaded. Recursive: " + std::to_string(config.watch_recursive));

    if (config.watch_paths.empty() && config.critical_paths.empty()) {
      logger.log(LOG_WARN, "No paths were provided. Using default ones...");
      config.watch_paths = {"/var/", "/etc/", ".", "/root/"};
      config.ignore_paths = {"/tmp/", "/var/"};
      config.critical_paths = {"/boot/", "/etc/"};
      config.watch_recursive = true;
    }
  } catch (const std::runtime_error& e) {
    logger.log(LOG_ERROR, e.what());
    logger.log(LOG_INFO, "Using default config values.");
  } catch (...) {
    logger.log(LOG_ERROR, "Reading from config failed for no reason.");
    logger.log(LOG_INFO, "Using default config values.");
  }

  int fd = inotify_init();
  int wd = inotify_add_watch(fd, "./", IN_CREATE | IN_DELETE);
  char buffer[4096];
  while (true) {
    int len = read(fd, buffer, sizeof(buffer));
    int i = 0;
    while (i < len) {
      auto* event = reinterpret_cast<inotify_event*>(&buffer[i]);
      if (event->len) {
        if (event->mask & IN_CREATE) {
          logger.log(LOG_INFO, "File was created: " + event->name);
        }
        if (event->mask & IN_DELETE) {
          logger.log(LOG_INFO, "File was deleted: " + event->name);
        }
      }
      i += sizeof(inotify_event) + event->len;
    }
  }
  return 0;
}
