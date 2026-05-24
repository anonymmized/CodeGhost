#include <gtest/gtest.h>

#include "core/watcher.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include "core/inotify_compat.hpp"
#include <unistd.h>

namespace {
    std::filesystem::path makeTempDir(const std::string& suffix) {
        auto dir = std::filesystem::temp_directory_path() / ("codeghost-test-" + suffix + "-" + std::to_string(::getpid()));

        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);

        return dir;
    }
}

TEST(WatcherIntegrationTest, ReceivesModifyEvent) {
#ifndef __linux__
    GTEST_SKIP() << "inotify watcher integration tests require Linux";
#endif
    const auto temp_dir = makeTempDir("modify");
    const auto watch_dir = temp_dir / "watch";
    const auto file_path = watch_dir / "modify-me.txt";

    std::filesystem::create_directories(watch_dir);

    {
        std::ofstream file(file_path);
        file << "initial";
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

    {
        std::ofstream file(file_path, std::ios::app);
        file << " updated";
    }

    char buffer[4096];
    const int bytes_read = read(watcher.getFd(), buffer, sizeof(buffer));

    ASSERT_GT(bytes_read, 0);

    bool saw_modify = false;
    std::string event_name;

    int offset = 0;
    while (offset < bytes_read) {
        auto* event = reinterpret_cast<inotify_event*>(buffer + offset);

        if ((event->mask & IN_MODIFY) && std::string(event->name) == "modify-me.txt") {
            saw_modify = true;
            event_name = event->name;
            break;
        }

        offset += sizeof(inotify_event) + event->len;
    }

    EXPECT_TRUE(saw_modify);
    EXPECT_EQ(event_name, "modify-me.txt");

    std::filesystem::remove_all(temp_dir);
}
