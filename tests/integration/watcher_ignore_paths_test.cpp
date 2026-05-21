#include <gtest/gtest.h>

#include "core/watcher.hpp"

#include <filesystem>
#include <string>
#include <unistd.h>

namespace {
    std::filesystem::path makeTempDir(const std::string& suffix) {
        auto dir = std::filesystem::temp_directory_path() / ("codeghost-test-" + suffix + "-" + std::to_string(::getpid()));

        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);

        return dir;
    }
}

TEST(WatcherIntegrationTest, RecursiveWatchSkipsIgnoredDirectory) {
    const auto temp_dir = makeTempDir("ignore");
    const auto watch_dir = temp_dir / "watch";
    const auto ignored_dir = watch_dir / "ignored";
    const auto normal_dir = watch_dir / "normal";

    std::filesystem::create_directories(ignored_dir);
    std::filesystem::create_directories(normal_dir);

    Config conf;
    conf.watch_paths = {watch_dir.string()};
    conf.ignore_paths = {ignored_dir.string()};
    conf.critical_paths = {};
    conf.start_hour = 0;
    conf.end_hour = 24;
    conf.watch_recursive = true;

    Watcher watcher(conf);
    watcher.registerRecursive(watch_dir.string());

    const auto watch_table = watcher.getWatchTable();

    bool has_watch_dir = false;
    bool has_ignored_dir = false;
    bool has_normak_dir = false;

    for (const auto& [wd, path] : watch_table) {
        if (path == watch_dir.string()) {
            has_watch_dir = true;
        }

        if (path == ignored_dir.string()) {
            has_ignored_dir = true;
        }

        if (path == normal_dir.string()) {
            has_normal_dir = true;
        }
    }

    EXPECT_TRUE(has_watch_dir);
    EXPECT_TRUE(has_normal_dir);
    EXPECT_FALSE(has_ignored_dir);

    std::filesystem::remove_all(temp_dir);
}
