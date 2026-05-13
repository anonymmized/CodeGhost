#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "daemon.hpp"

using json = nlohmann::ordered_json;

void daemonise(bool silent) {
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


void uploadToConfig(const Config& conf, const std::string& path) {
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
    std::ofstream outfile(path);
    if (!outfile.is_open()) {
        throw std::runtime_error("The config.json wasn't opened");
    }
    outfile << j.dump(4);
    outfile.close();
}

