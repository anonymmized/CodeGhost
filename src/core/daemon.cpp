#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <fstream>
#include <sys/inotify.h>

void daemonise(bool silent = true) {
    pid_t pid = fork(); // create subprocess
    if (pid < 0) exit(EXIT_FAILURE); // the proccess wasn't created
    if (pid > 0) {
        std::cout << "daemon PID: " << pid << '\n';
        exit(EXIT_SUCCESS); // ends current proccess with zero code
    }

    setsid(); // creates new session and proccess group. disconect with terminal
    if (silent) { // check on flag
        int fd = open("/dev/null", O_RDWR); // /dev/null opend for read and write
        // move all data from 0/1/2 to fd (/dev/null)
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);
    }
}

int main(int argc, char **argv) {
    int fd = inotify_init(); // initialise inotify saving file descriptor (fd)
    // add file descriptor to inotify watch function and sub on create and delete flags
    int wd = inotify_add_watch(fd, "./", IN_CREATE | IN_DELETE);
    char buffer[4096]; // create buffer to read
    if (argc == 2 && std::string(argv[1]) == "--daemonise") {
        std::cout << "daemon starting...\n";
        daemonise();
        while (true) {
            int len = read(fd, buffer, sizeof(buffer)); // read length to buffer
            int i = 0;
            while (i < len) {
                auto* event = reinterpret_cast<inotify_event*>(&buffer[i]); // inotify_event structure with main fields
                if (event->len) {
                    if (event->mask & IN_CREATE) {
                        std::ofstream infile("./logs.txt", std::ios::app);
                        infile << "Created: " << event->name << '\n'; // write logs to file
                        infile.close();
                    }
                    if (event->mask & IN_DELETE) {
                        std::ofstream infile("./logs.txt", std::ios::app);
                        infile << "Deleted: " << event->name << '\n';
                        infile.close();
                    }
                }
                i += sizeof(inotify_event) + event->len; // move index to end of the read segment
            }
        }
    }
    return 0;
}
