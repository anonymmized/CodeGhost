#include "Application/application.hpp"

#include <iostream>

int main(int argc, char** argv) {
    Application application(argc, argv);
    application.run();
    /*
    ParsedCommand command = parseCommandLine(argc, argv);
    std::cout << "Command name number: " << static_cast<int>(command.commandName) << '\n';
    if (command.pathToStart) {
        std::cout << "Path to start: " << *command.pathToStart << '\n';
    }
    if (command.pathToConfig) {
        std::cout << "Path to config: " << *command.pathToConfig << '\n';
    }
    if (command.indexToInteract) {
        std::cout << "Index to interact: " << *command.indexToInteract << '\n';
    }
    */
    return 0;
}
