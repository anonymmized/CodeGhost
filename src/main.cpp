#include "./cli/cli.hpp"
#include "./core/daemon.hpp"
#include "./core/hasher.hpp"
#include "./core/logger.hpp"
#include "./utils/utils.hpp"
#include <filesystem>

int main(int argc, char* argv[]) {
  CliArgs args = CliParser::parse(argc, argv);
  std::unordered_map<std::string, std::vector<std::string>> allin;
  if (!args.partsPath.empty()) {
    allin = parseICW(args.partsPath);
  }
  if (args.daemonise) daemonise();
  Logger logger(args.logPath, LOG_INFO, LOG_INFO, true, true);
  logger.log(LOG_INFO, "Logging to: " + args.logPath);
  logger.log(LOG_INFO, std::string(argv[0]) + " started.");
  Config config;
  try {
    config = loadFromConfig(args.configPath);
    logger.log(LOG_INFO, "Config is loaded. Recursive: " + std::to_string(config.watch_recursive));

    if (config.watch_paths.empty() && config.critical_paths.empty()) {
      logger.log(LOG_WARN, "No paths were provided. Using default ones...");
      config.watch_paths = allin["watch"];
      config.ignore_paths = allin["ignore"];
      config.critical_paths = allin["critical"];
      config.watch_recursive = true;
    }
  } catch (const std::runtime_error& e) {
    logger.log(LOG_ERROR, e.what());
    logger.log(LOG_INFO, "Using default config values.");
  } catch (...) {
    logger.log(LOG_ERROR, "Reading from config failed for no reason.");
    logger.log(LOG_INFO, "Using default config values.");
  }
  Watcher watcher(config);
  Hasher hasher(config.ignore_paths, config.watch_recursive);
  hasher.loadBaselineFile("baseline.json");
  logger.log(LOG_INFO, "Hashes were calculated.");
  if (config.watch_recursive) {
      for (const auto& path : config.watch_paths) {
        watcher.registerRecursive(path);
      }
  }
  char buffer[4096];
  while (true) {
    int len = read(watcher.getFd(), buffer, sizeof(buffer));
    int i = 0;
    while (i < len) {
      auto* event = reinterpret_cast<inotify_event*>(&buffer[i]);
      if (event->len) {
        if (!watcher.hasWatch(event->wd)) {
          i += sizeof(inotify_event) + event->len;
          continue;
        }
        std::string full_path = watcher.getFullPath(event->wd, std::string(event->name));
        if (event->mask & IN_CREATE) {
          if (std::filesystem::exists(full_path) && std::filesystem::is_directory(full_path))
            watcher.registerRecursive(full_path);
          hasher.fileChanged(full_path, logger);
        }
        if (event->mask & IN_MODIFY) {
          hasher.fileChanged(full_path, logger);
        }
        if (event->mask & IN_DELETE) {
          hasher.deleteHash(full_path, logger);
        }
        if (event->mask & IN_MOVED_FROM) {
          hasher.fileMoved(full_path, logger, false, event->cookie);
        }
        if (event->mask & IN_MOVED_TO) {
          hasher.fileMoved(full_path, logger, true, event->cookie);
        }
        if (event->mask & IN_DELETE) {
          watcher.removeWatcher(event->wd);
        }
      }
      i += sizeof(inotify_event) + event->len;
    }
  }
  return 0;
}
