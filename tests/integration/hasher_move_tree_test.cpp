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

TEST(HasherIntegrationTest, DirectoryMoveRekeysRuntimeEntries) {
    const auto temp_dir = makeTempDir("move");
    const auto old_dir = temp_dir / "old";
    const auto new_dir = temp_dir / "new";
    const auto old_file = old_dir / "tracked.txt";
    const auto baseline_path = temp_dir / "baseline.json";
    const auto log_path = temp_dir / "hasher.log";

    std::filesystem::create_directories(old_dir);
    {
        std::ofstream file(old_file);
        file << "content";
    }

    Config conf;
    conf.watch_paths = {temp_dir.string()};
    conf.watch_recursive = true;

    Hasher hasher({}, {}, true);
    hasher.initHashes(conf);
    hasher.saveBaseline(baseline_path.string());
    hasher.loadBaselineFile(baseline_path.string());

    std::filesystem::rename(old_dir, new_dir);

    Logger logger(log_path.string(), LOG_INFO, LOG_NONE, false, false);
    hasher.movePathTree(old_dir.string(), new_dir.string(), logger, true);
    hasher.syncBaseline(baseline_path.string());

    std::ifstream baseline_file(baseline_path);
    ASSERT_TRUE(baseline_file.is_open());

    std::string contents(
        (std::istreambuf_iterator<char>(baseline_file)),
        std::istreambuf_iterator<char>()
    );

    EXPECT_EQ(contents.find(old_file.string()), std::string::npos);
    EXPECT_NE(contents.find((new_dir / "tracked.txt").string()), std::string::npos);

    std::filesystem::remove_all(temp_dir);
}
