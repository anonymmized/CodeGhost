#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <fstream>
//#include <sys/inotify.h>
#include <vector>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <cstdint>
#include <chrono>
#include <iomanip>
#include <syslog.h>
#include "daemon/daemon.h"

using json = nlohmann::ordered_json;

int main(int argc, char **argv) {
    //TODO: config and argument parser should be in the beggining to initialize the following and the LOGFILE.

    std::string logpath = "test.log";
    LogSettings LOGSETTINGS;
    std::ofstream file(logpath, std::ios::app | std::ios::out);
    if (!file.is_open()) {
        openlog("daemon", LOG_PID | LOG_CONS, LOG_DAEMON);
        syslog(LOG_ERR, "Failed to open logfile: %s", logpath.c_str());
        std::cerr << "Failed to open logfile: " << logpath << '\n';
        closelog();
        return 1;
    }
    log(LOG_INFO, "Logging to: " + logpath, LOGSETTINGS, file);

    /*
    try {
        LOGFILE.open(logpath, std::ios::out | std::ios::app);
        log(LOG_INFO, "Logging to: " + logpath);
    } catch (...) {
        LOGSETTINGS.log_level = LOG_NONE; //not logging into file.
        log(LOG_ERROR, "Failed to open logfile for writing: "+ logpath );
    }
    */

    log(LOG_INFO, std::string(argv[0]) + " started.", LOGSETTINGS, file);

    try {
        config = loadFromConfig("./config.json");
        log(LOG_INFO, "Config is loaded. Recursive: " + std::to_string(config.watch_recursive));

       //init with default paths if none provided
        if ( config.watch_paths.empty() && config.critical_paths.empty() ){
            log(LOG_WARN, "No paths were provided. Using default ones...");
            config.watch_paths = {"/var/", "/etc/", ".", "/root"};
            config.ignore_paths = {"/tmp", "/var"};
            config.critical_paths = {"/boot/", "/etc/"};
            config.watch_recursive = true;
        }
    } catch (const std::runtime_error& e) {
        log(LOG_ERROR, e.what() );
        log(LOG_INFO, "Using default config values.");
    } catch (...) {
        log(LOG_ERROR, "Reading from config failed for no reason.");
        log(LOG_INFO, "Using default config values.");
    }

    /*
    int fd = inotify_init();
    int wd = inotify_add_watch(fd, "./", IN_CREATE | IN_DELETE);
    char buffer[4096];
    if (argc == 2 && std::string(argv[1]) == "--daemonise") {
        std::cout << "daemon starting...\n";
        daemonise();
        while (true) {
            int len = read(fd, buffer, sizeof(buffer));
            int i = 0;
            while (i < len) {
                auto* event = reinterpret_cast<inotify_event*>(&buffer[i]);
                if (event->len) {
                    if (event->mask & IN_CREATE) {
                        std::ofstream infile("./logs.txt", std::ios::app);
                        infile << "Created: " << event->name << '\n';
                        infile.close();
                    }
                    if (event->mask & IN_DELETE) {
                        std::ofstream infile("./logs.txt", std::ios::app);
                        infile << "Deleted: " << event->name << '\n';
                        infile.close();
                    }
                }
                i += sizeof(inotify_event) + event->len;
            }
        }
    }
    */

    return 0;
}
