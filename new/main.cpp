#include "Application/application.hpp"

#include <iostream>

int main(int argc, char** argv) {
    try {
        Application application(argc, argv);
        return application.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
