#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "./logger.hpp"

using json = nlohmann::ordered_json;

struct Alert {
    std::string file_path;
    uint64_t old_hash = 0;
    uint64_t new_hash = 0;
    std::string detected_at;
    std::string reason;
    std::string status = "pending";
};

enum Action { CHANGE, REMOVE };

class Notifier {
public:
    Notifier(const std::string& webhook_url, int timeout_sec = 5);

    bool send(const Alert& alert, Logger& logger);
    bool sendOrQueue(const Alert& alert, const std::string& pending_path, Logger& logger);
    void retryPending(const std::string& pending_path, Logger& logger);
    void init(const std::string& pending_path);

    std::vector<std::string> pollApprovals(Logger& logger);

private:
    std::string host_;
    std::string endpoint_;
    int timeout_sec_;
    std::vector<Alert> pending_;

    void parseUrl(const std::string& url);
    std::string getTimestampUTC() const;
    json toJson(const Alert& a) const;
    Alert fromJson(const json& j) const;
    bool changePending(const Alert& alert, Action action, const std::string& pending_path);
    std::vector<Alert> loadPending(const std::string& path);
    bool savePending(const std::vector<Alert>& alerts, const std::string& path);
};
