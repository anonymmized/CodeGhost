#include "Application/application.hpp"
#include "Resources/info.hpp"
#include "Settings/configLoader.hpp"

#include <string>
#include <charconv>
#include <iostream>

namespace {
    std::string buildLine(int argc, char* argv[]) {
        std::string finLine;
        int i = 0;
        while (i < argc) {
            finLine += argv[i++];
            finLine += ' ';
        }
        return finLine;
    }

    std::filesystem::path parseArgumentPath(const std::string& argument) {
        if (argument.empty()) {
            throw std::invalid_argument("There is no target path");
        }
        return std::filesystem::path(std::string(argument));
    }

    std::uint64_t parseArgumentIndex(const std::string& argument) {
        if (argument.empty()) {
            throw std::invalid_argument("No target ID");
        }
        std::uint64_t value{};
        const char* begin = argument.data();
        const char* end = begin + argument.size();

        const auto [ptr, error] = std::from_chars(begin, end, value);
        if (error != std::errc{} || ptr != end) {
            throw std::invalid_argument("The ID must contain just decimal digits");
        }
        return value;
    }

    void throwBadIndex(int argc) {
        if (argc != 3) {
            throw std::invalid_argument("Bad or empty index");
        }
    }

    void throwDefaultArgs(int argc) {
        if (argc != 2) {
            throw std::invalid_argument("No arguments allowed");
        }
    }
};

ParsedCommand parseCommandLine(int argc, char* argv[]) {
    ParsedCommand command;
    if (argc < 2) {
        return command;
    }
    static const std::unordered_map<std::string, CommandName> commands {
        {"help",      CommandName::Help},
        {"version",   CommandName::Version},
        {"v",         CommandName::Version},
        {"start",     CommandName::Start},
        {"check_cfg", CommandName::CheckConfig},
        {"cc",        CommandName::CheckConfig},
        {"list",      CommandName::List},
        {"ls",        CommandName::List},
        {"stop",      CommandName::Stop},
        {"baseline",  CommandName::CreateBaseline},
        {"bsl",       CommandName::CreateBaseline},
        {"pause",     CommandName::Pause},
        {"resume",    CommandName::Resume},
        {"add_cfg",   CommandName::AddConfig},
        {"ac",        CommandName::AddConfig}
    };
    const auto commandWord = commands.find(argv[1]);
    if (commandWord == commands.end()) {
        throw std::invalid_argument("Bad argument: " + std::string(argv[1]));
    }
    command.commandName = commandWord->second;
    std::string argv_str = buildLine(argc, argv);
    switch (command.commandName) {
        case CommandName::Start:
            if (argc != 3) {
                throw std::invalid_argument("add_cfg accepts at most one path");
            }
            command.pathToStart = parseArgumentPath(argv[2]);
            break;
        
        case CommandName::AddConfig: {
            if (argc > 3) {
                throw std::invalid_argument("add_cfg accepts at most one path");
            }
            if (argc == 3) {
                command.pathToConfig = parseArgumentPath(argv[2]);
            }
            break;
        }

        case CommandName::Stop:
        case CommandName::CreateBaseline:
        case CommandName::Pause:
        case CommandName::Resume:
            throwBadIndex(argc);
            command.indexToInteract = parseArgumentIndex(argv[2]);
            break;
        default:
            throwDefaultArgs(argc);
            break;
    }
    return command;
}

int Application::run() {
    switch (command_.commandName) {
        case CommandName::Help:
            printHelp();
            break;
        case CommandName::Version:
            printVersion();
            break;
        case CommandName::AddConfig: {
            Config config{};
            if (command_.pathToConfig) {
                config = getConfig(*command_.pathToConfig);
            }
            saveConfig(config);
            std::cout << "Configuration saved\n";
            break;
        }
        case CommandName::CheckConfig: {
            getConfig();
            std::cout << "Configuration is valid\n";
            break;
        }
        default:
            std::cerr << "Command is not implemented yet\n";
            return 1;
    }
    return 0;
}
