#include "Storage/baselineStorage.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <string>
#include <chrono>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <system_error>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <unistd.h>

namespace {
    void validateBaseline(const nlohmann::json& data) {
        if (!data.is_object()) {
            throw std::runtime_error("Baseline must be an object");
        }
        const auto& version = data.at("formatVersion");
        if (!version.is_number_integer() || version != 1) {
            throw std::runtime_error("Unsupported baseline format");
        }

        const auto& algorithm = data.at("hashAlgorithm");
        if (!algorithm.is_string() || algorithm != "sha256") {
            throw std::runtime_error("Unsupported hash algorithm");
        }

        if (!data.at("files").is_object()) {
            throw std::runtime_error("Baseline files must be an object");
        }
    }
    
    void validateBaselineEntry(const std::string& pathString, const nlohmann::json& entry) {
        const std::filesystem::path path{pathString};
        if (path.empty() || !path.is_absolute() || pathString.find('\0') != std::string::npos) {
            throw std::runtime_error("Baseline path must be absolute and contain no null bytes");
        }
        if (!entry.is_object()) {
            throw std::runtime_error("File entry must be an object");
        }

        const auto& hash = entry.at("hash");
        if (!hash.is_string()) {
            throw std::runtime_error("Hash must be a string");
        }
        const auto& hashString = hash.get_ref<const std::string&>();
        const bool validHash = hashString.size() == 64 &&
            std::all_of(hashString.begin(), hashString.end(), [](char character) {
                return (character >= '0' && character <= '9') ||
                       (character >= 'a' && character <= 'f');
            });
        if (!validHash) {
            throw std::runtime_error("Hash must contain 64 lowercase hexadecimal characters");
        }

        const auto& size = entry.at("size");
        if (!size.is_number_integer()) {
            throw std::runtime_error("Size must be an integer");
        }
        if (!size.is_number_unsigned() && size.get<std::int64_t>() < 0) {
            throw std::runtime_error("Size must not be negative");
        }
        if (size.get<std::uint64_t>() > std::numeric_limits<std::uintmax_t>::max()) {
            throw std::runtime_error("Size is out of range");
        }

        const auto& permissions = entry.at("permissions");
        if (!permissions.is_number_integer()) {
            throw std::runtime_error("Permissions must be an integer");
        }
        if (!permissions.is_number_unsigned() && permissions.get<std::int64_t>() < 0) {
            throw std::runtime_error("Permissions must not be negative");
        }
        const auto permissionBits = permissions.get<std::uint64_t>();
        const auto allowedBits = static_cast<std::uint64_t>(std::filesystem::perms::mask);
        if ((permissionBits & ~allowedBits) != 0) {
            throw std::runtime_error("Permissions contain unsupported bits");
        }

        const auto& modified = entry.at("modifiedAtNs");
        if (!modified.is_number_integer()) {
            throw std::runtime_error("Modification time must be an integer");
        }
        if (modified.is_number_unsigned() && modified.get<std::uint64_t>() > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
            throw std::runtime_error("Modification time is out of range");
        }
    }

    void writeBaselineAutomically(const nlohmann::json& data, const std::filesystem::path& storagePath) {
        const std::string text = data.dump(4) + '\n';
        if (storagePath.empty() || storagePath.native().find('\0') != std::string::npos) {
            throw std::runtime_error("Invalid baseline storage path");
        }
        auto tempName = storagePath.string() + ".tmp.XXXXXX";
        const int descriptor = ::mkstemp(tempName.data());
        if (descriptor == -1) {
            throw std::system_error(errno, std::generic_category(), "Cannot create temporary baseline");
        }
        std::FILE* tempFile = ::fdopen(descriptor, "wb");
        if (!tempFile) {
            const int error = errno;
            ::close(descriptor);
            ::unlink(tempName.c_str());
            throw std::system_error(error, std::generic_category(), "Cannot open temporary baseline stream");
        }
        try {
            if (std::fwrite(text.data(), 1, text.size(), tempFile) != text.size()) {
                throw std::runtime_error("Failed to write temporary baseline");
            }
            const int closeResult = std::fclose(tempFile);
            tempFile = nullptr;
            if (closeResult != 0) {
                throw std::runtime_error("Failed to close temporary baseline");
            }
            std::filesystem::rename(tempName, storagePath);
        } catch (...) {
            if (tempFile) {
                std::fclose(tempFile);
            }
            ::unlink(tempName.c_str());
            throw;
        }
    }
};

void saveBaseline(const std::filesystem::path& storagePath, const ScanPoint& scanResult) {
    if (!scanResult.errors.empty()) {
        throw std::runtime_error("Cannot save an incomplete baseline");
    }
    nlohmann::json data;
    data["formatVersion"] = 1;
    data["hashAlgorithm"] = "sha256";
    data["files"] = nlohmann::json::object();
    for (const auto& scan : scanResult.files) {
        auto str_path = scan.path.string();
        if (data["files"].contains(str_path)) {
            throw std::runtime_error("Duplicate baseline path: " + str_path);
        }
        data["files"][str_path]["hash"] = scan.contentHash;
        data["files"][str_path]["size"] = scan.size;

        const auto systemTime = std::filesystem::file_time_type::clock::to_sys(scan.modificationTime);
        using SystemDuration = decltype(systemTime.time_since_epoch());
        using WideNanoseconds = std::chrono::duration<SystemDuration::rep, std::nano>;
        const auto nanoseconds = std::chrono::duration_cast<WideNanoseconds>(systemTime.time_since_epoch()).count();
        if (nanoseconds < std::numeric_limits<std::int64_t>::min() ||
            nanoseconds > std::numeric_limits<std::int64_t>::max()) {
            throw std::runtime_error("Modification time is out of range");
        }
        data["files"][str_path]["modifiedAtNs"] = static_cast<std::int64_t>(nanoseconds);
        data["files"][str_path]["permissions"] = static_cast<unsigned int>(scan.permissions);
        validateBaselineEntry(str_path, data["files"][str_path]);
    }
    writeBaselineAutomically(data, storagePath);
}

std::vector<FileRecord> loadBaseline(const std::filesystem::path& storagePath) {
    std::ifstream baselinePath(storagePath);
    if (!baselinePath.is_open()) {
        throw std::runtime_error("Cannot open file: " + storagePath.string());
    }
    std::vector<std::set<std::string>> objectKeys;
    auto rejectDuplicates = [&objectKeys](int, nlohmann::json::parse_event_t event, nlohmann::json& value) {
        using Event = nlohmann::json::parse_event_t;
        if (event == Event::object_start) {
            objectKeys.emplace_back();
        } else if (event == Event::key) {
            if (!objectKeys.back().insert(value.get<std::string>()).second) {
                throw std::runtime_error("Duplicate JSON key in baseline");
            }
        } else if (event == Event::object_end) {
            objectKeys.pop_back();
        }
        return true;
    };
    const auto data = nlohmann::json::parse(baselinePath, rejectDuplicates);
    if (baselinePath.bad()) {
        throw std::runtime_error("Failed to read baseline");
    }
    validateBaseline(data);
    std::vector<FileRecord> result;
    for (const auto& [pathString, entry] : data.at("files").items()) {
        validateBaselineEntry(pathString, entry);
        FileRecord fileRecord{};
        fileRecord.path = pathString;
        fileRecord.contentHash = entry.at("hash").get<std::string>();
        fileRecord.size = entry.at("size").get<std::uintmax_t>();
        fileRecord.permissions = static_cast<std::filesystem::perms>(entry.at("permissions").get<unsigned int>());

        const auto nanoseconds = entry.at("modifiedAtNs").get<std::int64_t>();
        const std::chrono::sys_time<std::chrono::nanoseconds> systemTime{std::chrono::nanoseconds{nanoseconds}};
        fileRecord.modificationTime = std::filesystem::file_time_type::clock::from_sys(systemTime);
        result.push_back(fileRecord);
    }
    return result;
}
