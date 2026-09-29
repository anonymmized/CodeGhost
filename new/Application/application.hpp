#pragma once

#include <optional>
#include <filesystem>
#include <cstdint>

enum class CommandName {
    Help,
    Version,
    CheckConfig,
    Start,
    List,
    Stop,
    CreateBaseline,
    Pause,
    Resume,
    AddConfig
};

struct ParsedCommand {
    CommandName commandName = CommandName::Help;

    std::optional<std::filesystem::path> pathToStart;
    std::optional<std::filesystem::path> pathToConfig;
    std::optional<std::uint64_t> indexToInteract;
};

ParsedCommand parseCommandLine(int argc, char* argv[]);

class Application {
    private:
        ParsedCommand command_;
    public:
        Application(int argc, char* argv[]) : command_(parseCommandLine(argc, argv)) {}
        int run();
};
