#include <gtest/gtest.h>

#include "core/watcher.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include "core/inotify_compat.hpp"
#include <unistd.h>
#include <vector>

namespace {
std::filesystem::path makeTempDir() {
    auto base = std::filesystem::temp_directory_path();
    auto dir = base / ("codeghost-test-" + std::to_string(::getpid()));

    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);

    return dir;
}

}


TEST(FilesystemMonitoringIntegrationTest, WatcherReceivesCreateEvent) {
#ifndef __linux__
    GTEST_SKIP() << "inotify watcher integration tests require Linux";
#endif
    const auto temp_dir = makeTempDir();
    const auto watch_dir = temp_dir / "watch";

    std::filesystem::create_directories(watch_dir);

    Config conf;
    conf.watch_paths = {watch_dir.string()};
    conf.ignore_paths = {};
    conf.critical_paths = {};
    conf.start_hour = 0;
    conf.end_hour = 24;
    conf.watch_recursive = false;

    Watcher watcher(conf);
    watcher.addWatch(watch_dir.string());

    const auto file_path = watch_dir / "test.txt";

    {
        std::ofstream file(file_path);
        file << "hello";
    }

    char buffer[4096];
    const int bytes_read = read(watcher.getFd(), buffer, sizeof(buffer));
    ASSERT_GT(bytes_read, 0);
    auto* event = reinterpret_cast<inotify_event*>(buffer);
    EXPECT_TRUE(event->mask & IN_CREATE);
    EXPECT_STREQ(event->name, "test.txt");

    std::filesystem::remove_all(temp_dir);
}
