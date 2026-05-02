#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <string>

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

int main(int argc, char **argv) {
    if (argc == 2 && std::string(argv[1]) == "--daemonise") {
        std::cout << "daemon starting...\n";
        daemonise();
        while (true) {
            sleep(10);
        }
    }
    return 0;
}
