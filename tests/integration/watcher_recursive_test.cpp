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

TEST(WatcherIntegrationTest, RecursiveWatchReceivesCreateEventInSubdirectory) {
    const auto temp_dir = makeTempDir("recursive");
    const auto watch_dir = temp_dir / "watch";
    const auto nested_dir = watch_dir / "nested";
    const auto file_path = nested_dir / "inside.txt";

    std::filesystem::create_directories(nested_dir);

    Config conf;
    conf.watch_paths = {watch_dir.string()};
    conf.ignore_paths = {};
    conf.critical_paths = {};
    conf.start_hour = 0;
    conf.end_hour = 24;
    conf.watch_recursive = true;

    Watcher watcher(conf);
    watcher.registerRecursive(watch_dir.string());

    {
        std::ofstream file(file_path);
        file << "nested content";
    }

    char buffer[4096];
    const int bytes_read = read(watcher.getFd(), buffer, sizeof(buffer));

    ASSERT_GT(bytes_read, 0);

    bool saw_create = false;
    std::string event_name;

    int offset = 0;
    while (offset < bytes_read) {
        auto* event = reinterpret_cast<inotify_event*>(buffer + offset);

        if ((event->mask & IN_CREATE) && std::string(event->name) = "inside.txt") {
            saw_create = true;
            event_name = event->name;
            break;
        }

        offset += sizeof(inotify_event) + event->len;
    }

    EXPECT_TRUE(saw_create);
    EXPECT_EQ(event_name, "inside.txt");

    std::filesystem::remove_all(temp_dir);
}
