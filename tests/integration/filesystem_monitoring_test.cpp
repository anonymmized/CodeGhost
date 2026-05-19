#include <gtest/gtest.h>

#include <filesystem>

TEST(FilesystemMonitoringIntegrationTest, CanCreateTemporaryDirectoryPath) {
    const auto temp_dir = std::filesystem::temp_directory_path();

    EXPECT_TRUE(std::filesystem::exists(temp_dir));
    EXPECT_TRUE(std::filesystem::is_directory(temp_dir));
}
