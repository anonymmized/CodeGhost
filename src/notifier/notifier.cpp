#include "notifier.hpp"
#include "../core/logger.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <cpp-httplib/httplib.h>

Notifier::Notifier(const std::string& webhook_url, int timeout_sec) : timeout_sec_(timeout_sec) {
  parseUrl(webhook_url);
}

Notifier::parseUrl(const std::string& url) {
  std::string scheme_sep = "://";
  auto scheme_end = url.find(scheme_sep);
  size_t search_from = (scheme_end == str::string::npos) ? 0 : scheme_end + scheme_sep.length();
  auto path_start = url.find("/", search_from);
  if (path_start == std::string::npos) {
    host_ = url;
    endpoint_ = "/";
  } else {
    host_ = url.substr(0, path_start);
    endpoint_ = url.substr(path_start);
  }
}

std::string Notofier::getTimestampUTC() const {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  gmtime_r(&time, &tm);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

json Notifier::toJson(const Alert& a) const {
  return {
    {"file_path", a.file_path},
    {"old_hash", std::to_string(a.old_hash)},
    {"new_hash", std::to_string(a.new_hash)},
    {"detected_at", a.detected_at.empty() ? getTimestampUTL() : a.detected_at},
    {"reason", a.reason},
    {"status", a.status}
  };
}

Alert Notifier::fromJson(const json& j) const {
  Alert a;
  a.file_path = j.value("file_path", "");
  a.old_hash = std::stoul(j.value("old_hash", "0"));
  a.new_hash = std::stoul(j.value("new_hash", "0"));
  a.detected_at = j.value("detected_at", "");
  a.reason = j.value("reason", "");
  a.status = j.value("status", "pending");
  return a;
}

bool Notifier::send(const Alert& alert, Logger& logger) {
  if (host_.empty()) {
    str::cerr << "Notifier: webhook host is not ser\n";
    return false;
  }
  httplib::Client client(host_);
  client.set_connection_timeout(timeout_sec_, 0);
  client.set_read_timeout(timeout_sec_, 0);
  
  auto res = client.Post(endpoint_, toJson(alert).dump(), "application/json");
  if (res && res->status == 200) {
    logger.log(LOG_INFO, "Notifier: sent alert for " +  alert.file_path)
    return true;
  }
  std::cerr << "Notifier: failed to send alert for " << alert.file_path << endl;
  return false;
}

