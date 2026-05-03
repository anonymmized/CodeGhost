#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <fstream>
//#include <sys/inotify.h>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

struct Config {
    std::vector<std::string> watch_paths;
    std::vector<std::string> ignore_paths;
    std::vector<std::string> critical_paths;
    int start_hour;
    int end_hour;
    bool recursive = true;
};

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

/*
Config loadFromConfig(const std::string& path) {

    Config conf;

}
*/

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
    j["recursive"] = conf.recursive;
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

    Config conf = {{".", "../"}, {"/tmp", "/var"}, {"/root", "/user"}, 10, 20, true};
    uploadToConfig(conf);
    std::cout << "The config was upload to file\n";
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
