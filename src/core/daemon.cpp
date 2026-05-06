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

using json = nlohmann::ordered_json;

void daemonise(bool silent = true) {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) {
        std::cout << "daemon PID: " << pid << '\n';
        exit(EXIT_SUCCESS);
    }

    setsid();
    if (silent) {
        int fd = open("/dev/null", O_RDWR);
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);
    }
}


Config loadFromConfig(const std::string& path) {
    std::ifstream infile(path);
    if (!infile.is_open()) {
        throw std::runtime_error("The config.json wasn't opened"); //TODO: check if file doesn't exist, throw relevant exception.
    }
    json j;
    infile >> j;
    Config conf;
    conf.watch_paths = j["watch_paths"].get<std::vector<std::string>>();
    conf.ignore_paths = j["ignore_paths"].get<std::vector<std::string>>();
    conf.critical_paths = j["critical_paths"].get<std::vector<std::string>>();
    conf.start_hour = j["start_hour"];
    conf.end_hour = j["end_hour"];
    conf.watch_recursive = j["watch_recursive"];
    infile.close();
    return conf;
}


void uploadToConfig(const Config& conf) {
    json j;
    for (int i = 0; i < conf.watch_paths.size(); i++) {
        j["watch_paths"].push_back(conf.watch_paths[i]);
    }
    for (int i = 0; i < conf.ignore_paths.size(); i++) {
        j["ignore_paths"].push_back(conf.ignore_paths[i]);
    }
    for (int i = 0; i < conf.critical_paths.size(); i++) {
        j["critical_paths"].push_back(conf.critical_paths[i]);
    }
    j["start_hour"] = conf.start_hour;
    j["end_hour"] = conf.end_hour;
    j["watch_recursive"] = conf.watch_recursive;
    std::ofstream outfile("config.json");
    if (!outfile.is_open()) {
        throw std::runtime_error("The config.json wasn't opened");
    }
    outfile << j.dump(4);
    outfile.close();
}
/*
std::vector<std::string> getArgs(const char **argv) {

}
*/

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

