#pragma once

// #include "../core/logger.hpp"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using = nlohmann::json;

struct Alert {
  std::string file_path;
  uint64_t old_hash;
  uint64_t new_hash;
  std::string detected_at;
  std::string reason;
  std::string status;
}

enum class Action {
  REMOVE,
  CHANGE
}

class Notifier {
public:
  explicit Notifier(const std::string& webhook_url, int timeout_sec = 5);
  void init(const std::string& pending_path);
  bool sendOrQueue(const Alert& alert, const std::string& pending_path);
  void retryPending(const std::string& pending_path);

private:
  std::vector<Alert> pending_;
  std::string host_;
  std::string endpoint_;
  int timeout_sec_;
  
  bool send(const Alert& alert);
  bool searchAlert(const Alert& alert);
  std::vector<Alert> loadPending(const std::string& path);
  bool savePending(const std::vector<Alert>& alerts, const std::string& path);
  json toJson(const Alert& a) const;
  Alert fromJson(const json& j) const;
  void parseUrl(const std::string& url);
  std::string getTimestampUTC() const;
}
