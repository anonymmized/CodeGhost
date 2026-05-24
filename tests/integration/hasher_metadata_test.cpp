#include <gtest/gtest.h>

#include "core/hasher.hpp"
#include "core/logger.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace {
std::filesystem::path makeTempDir(const std::string& suffix) {
    auto dir = std::filesystem::temp_directory_path() / ("codeghost-hasher-" + suffix + "-" + std::to_string(::getpid()));
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}
}

TEST(HasherIntegrationTest, MetadataChangeIsPersistedWithoutHashChange) {
    const auto temp_dir = makeTempDir("metadata");
    const auto file_path = temp_dir / "tracked.txt";
    const auto baseline_path = temp_dir / "baseline.json";
    const auto log_path = temp_dir / "hasher.log";

    {
        std::ofstream file(file_path);
        file << "content";
    }

    Config conf;
    conf.watch_paths = {temp_dir.string()};
    conf.watch_recursive = false;

    Hasher hasher({}, {}, false);
    hasher.initHashes(conf);
    hasher.saveBaseline(baseline_path.string());
    hasher.loadBaselineFile(baseline_path.string());

    std::filesystem::permissions(
        file_path,
        std::filesystem::perms::owner_read,
        std::filesystem::perm_options::replace
    );

    Logger logger(log_path.string(), LOG_INFO, LOG_NONE, false, false);
    hasher.fileAttributed(file_path.string(), logger);
    hasher.syncBaseline(baseline_path.string());

    std::ifstream baseline_file(baseline_path);
    ASSERT_TRUE(baseline_file.is_open());

    std::string contents(
        (std::istreambuf_iterator<char>(baseline_file)),
        std::istreambuf_iterator<char>()
    );

    EXPECT_NE(contents.find("\"permissions\""), std::string::npos);
    EXPECT_NE(contents.find(file_path.string()), std::string::npos);

    std::filesystem::remove_all(temp_dir);
}
