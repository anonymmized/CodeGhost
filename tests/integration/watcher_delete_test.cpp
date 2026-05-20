#include <gtest/gtest.h>

#include "core/watcher.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <sys/inotify.h>
#include <unistd.h>

namespace {
    std::filesystem::path makeTempDir(const std::string& suffix) {
        auto dir = std::filesystem::temp_directory_path() / ("codeghost-test-" + suffix + "-" + std::to_string(::getpid()));

        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);

        return dir;
    }
}

TEST(WatcherIntegrationTest, ReceivesDeleteEvent) {
    const auto temp_dir = makeTempDir("delete");
    const auto watch_dir = temp_dir / "watch";
    const auto file_path = watch_dir / "delete-me.txt";

    std::filesystem::create_directories(watch_dir);

    {
        std::ofstream file(file_path);
        file << "content";
    }

    Config conf;
    conf.watch_paths = {watch_dir.string()};
    conf.ignore_paths = {};
    conf.critical_paths = {};
    conf.start_hour = 0;
    conf.end_hour = 24;
    conf.watch_recursive = false;

    Watcher watcher(conf);
    watcher.addWatch(watch_dir.string());

    std::filesystem::remove(file_path);

    char buffer[4096];
    const int bytes_read = read(watcher.getFd(), buffer, sizeof(buffer));

    ASSERT_GT(bytes_read, 0);

    auto* event = reinterpret_cast<inotify_event*>(buffer);

    EXPECT_TRUE(event->mask & IN_DELETE);
    EXPECT_STREQ(event->name, "delete-me.txt");

    std::filesystem::remove_all(temp_dir);
}
