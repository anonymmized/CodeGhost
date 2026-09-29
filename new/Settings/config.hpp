#pragma once

struct Config {
    bool recursive = false;
    std::vector<std::filesystem::path> ignorePaths;
    std::chrono::seconds scanInterval{300};
};
