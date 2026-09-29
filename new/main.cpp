#include "Application/application.hpp"

int main(int argc, char** argv) {
    /*
    Application application(argc, argv);
    application.run();
    */
    ParsedCommand command = parseCommandLine(argc, argv);
}
