#include "Application/application.hpp"
#include "Inspector/scanner.hpp"
#include "Storage/baselineStorage.hpp"

#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

namespace {
    namespace fs = std::filesystem;

    void require(bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    template<class Function>
    void expectFailure(Function action) {
        try {
            action();
        } catch (const std::exception&) {
            return;
        }
        throw std::runtime_error("Expected an exception");
    }

    struct TemporaryDirectory {
        fs::path path;

        TemporaryDirectory() {
            auto pattern = (fs::temp_directory_path() / "codeghost-tests-XXXXXX").string();
            require(::mkdtemp(pattern.data()) != nullptr, "Cannot create test directory");
            path = pattern;
        }

        ~TemporaryDirectory() {
            std::error_code error;
            fs::remove_all(path, error);
        }
    };

    void writeText(const fs::path& path, const std::string& text) {
        std::ofstream out(path, std::ios::binary);
        out << text;
        out.close();
        require(static_cast<bool>(out), "Cannot write test file");
    }

    std::string readText(const fs::path& path) {
        std::ifstream in(path, std::ios::binary);
        require(in.is_open(), "Cannot read test file");
        return {std::istreambuf_iterator<char>(in), {}};
    }

    void requireNoTemporaryBaselines(const fs::path& directory) {
        for (const auto& entry : fs::directory_iterator(directory)) {
            require(entry.path().filename().string().find(".tmp.") == std::string::npos,
                    "Temporary baseline was not removed");
        }
    }

    void testScanner(const fs::path& root) {
        const auto tree = root / "tree";
        fs::create_directories(tree / "skip");
        fs::create_directories(tree / "skip2");
        writeText(tree / "top", "abc");
        writeText(tree / "skip" / "nested", "abc");
        writeText(tree / "skip2" / "keep", "abc");
        fs::create_symlink(tree / "top", tree / "link");

        Config config{};
        auto result = Scanner(config).scan({tree});
        require(result.errors.empty() && result.files.size() == 1, "Nonrecursive scan failed");

        config.recursive = true;
        config.ignorePaths = {tree / "skip" / ""};
        result = Scanner(config).scan({tree});
        require(result.errors.empty() && result.files.size() == 2, "Ignore boundaries failed");
        for (const auto& file : result.files) {
            require(file.size == 3 && file.contentHash ==
                "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
                "Wrong hash or size");
        }
        result = Scanner(config).scan({tree / "skip" / "nested", tree / "link"});
        require(result.errors.empty() && result.files.empty(), "Excluded input was scanned");
        result = Scanner(config).scan({tree / "missing", tree / "top"});
        require(result.errors.size() == 1 && result.files.size() == 1, "Missing input handling failed");
    }

    void testBaseline(const fs::path& root) {
        const auto path = root / "baseline.json";
        FileRecord record{};
        record.path = root / "not-required-to-exist";
        record.contentHash = std::string(64, 'a');
        record.size = std::numeric_limits<std::uintmax_t>::max();
        record.permissions = fs::perms::owner_read | fs::perms::owner_write;
        record.modificationTime = fs::file_time_type::clock::from_sys(
            std::chrono::sys_time<std::chrono::nanoseconds>{std::chrono::nanoseconds{-123456789}});
        ScanPoint scan{{record}, {}};

        saveBaseline(path, scan);
        const auto loaded = loadBaseline(path);
        require(loaded.size() == 1, "Wrong record count");
        const auto& actual = loaded.front();
        require(actual.path == record.path && actual.contentHash == record.contentHash &&
                actual.size == record.size && actual.permissions == record.permissions &&
                actual.modificationTime == record.modificationTime, "Baseline roundtrip failed");

        const auto saved = readText(path);
        auto duplicate = scan;
        duplicate.files.push_back(record);
        expectFailure([&] { saveBaseline(path, duplicate); });
        require(readText(path) == saved, "Duplicate destroyed baseline");
        auto incomplete = scan;
        incomplete.errors.push_back({root / "missing", "Cannot read"});
        expectFailure([&] { saveBaseline(path, incomplete); });
        require(readText(path) == saved, "Incomplete scan destroyed baseline");
        auto invalid = scan;
        invalid.files.front().contentHash = "invalid";
        expectFailure([&] { saveBaseline(path, invalid); });
        require(readText(path) == saved, "Invalid record destroyed baseline");

        // On macOS file_time_type has a wider range than the int64 JSON format.
        if constexpr (sizeof(fs::file_time_type::duration::rep) > sizeof(std::int64_t)) {
            invalid = scan;
            invalid.files.front().modificationTime = fs::file_time_type::clock::from_sys(
                std::chrono::sys_time<std::chrono::seconds>{std::chrono::seconds{10000000000LL}});
            expectFailure([&] { saveBaseline(path, invalid); });
            require(readText(path) == saved, "Out-of-range timestamp destroyed baseline");
        }

        const auto valid = nlohmann::json::parse(saved);
        auto reject = [&](const nlohmann::json& data) {
            writeText(path, data.dump());
            expectFailure([&] { loadBaseline(path); });
        };
        auto data = valid;
        data["formatVersion"] = 999;
        reject(data);
        data = valid;
        data["hashAlgorithm"] = "md5";
        reject(data);
        data = valid;
        data["files"] = nullptr;
        reject(data);
        for (const auto& [key, value] : std::vector<std::pair<std::string, nlohmann::json>>{
                {"hash", "bad"}, {"hash", std::string(64, 'g')}, {"hash", 123},
                {"size", -1}, {"size", 1.5}, {"permissions", -1}, {"permissions", 4096},
                {"modifiedAtNs", std::numeric_limits<std::uint64_t>::max()},
                {"modifiedAtNs", 1.5}}) {
            data = valid;
            data["files"][record.path.string()][key] = value;
            reject(data);
        }
        data = valid;
        data["files"]["relative"] = valid["files"][record.path.string()];
        reject(data);
        data = valid;
        data["files"][record.path.string()].erase("size");
        reject(data);
        writeText(path, "{\"formatVersion\":1,\"formatVersion\":1,\"hashAlgorithm\":\"sha256\",\"files\":{}}");
        expectFailure([&] { loadBaseline(path); });
        writeText(path, saved + "garbage");
        expectFailure([&] { loadBaseline(path); });

        // A stale fixed-name temporary symlink must never be followed or deleted.
        const auto victim = root / "victim";
        writeText(victim, "unchanged");
        fs::create_symlink(victim, path.string() + ".tmp");
        saveBaseline(path, scan);
        require(readText(victim) == "unchanged", "Temporary symlink target was overwritten");
        require(fs::is_symlink(path.string() + ".tmp"), "Unrelated temporary file was deleted");

        const auto blocked = root / "blocked";
        fs::create_directory(blocked);
        writeText(blocked / "keep", "unchanged");
        expectFailure([&] { saveBaseline(blocked, scan); });
        require(readText(blocked / "keep") == "unchanged", "Rename failure damaged destination");
        requireNoTemporaryBaselines(root);

        // Writers use separate temporary files. The last successful rename wins.
        std::vector<std::exception_ptr> failures(4);
        std::vector<std::thread> writers;
        for (std::size_t i = 0; i < failures.size(); ++i) {
            writers.emplace_back([&, i] {
                try {
                    auto current = scan;
                    current.files.front().size = i;
                    saveBaseline(path, current);
                } catch (...) {
                    failures[i] = std::current_exception();
                }
            });
        }
        for (auto& writer : writers) {
            writer.join();
        }
        for (const auto& failure : failures) {
            if (failure) {
                std::rethrow_exception(failure);
            }
        }
        require(loadBaseline(path).front().size < failures.size(), "Concurrent save corrupted baseline");
        requireNoTemporaryBaselines(root);
        saveBaseline(path, ScanPoint{});
        require(loadBaseline(path).empty(), "Empty baseline roundtrip failed");
    }
}

int main() {
    try {
        TemporaryDirectory temporary;
        testScanner(temporary.path);
        testBaseline(temporary.path);
        std::cout << "Scanner and baseline regression checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
