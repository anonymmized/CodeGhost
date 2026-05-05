#pragma once
#include <string>

enum class CompareResult {
    MATCH,
    CHANGED,
    NEW_FILE,
    ERROR
};

struct CompareReport {
    CompareResult result;
    std::string filePath;
    std::string currentHash;
    std::string verifiedHash;
    std::string error;
};

class HashComparator {
public:
    static std::string computeHash(const std::string& filePath);
    static CompareReport compare(const std::string& filePath, const std::string& verifiedHash);
};
