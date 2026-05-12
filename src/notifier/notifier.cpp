#include "notifier.hpp"
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

void Notifier::parseUrl(const std::string& url) {
  std::string scheme_sep = "://";
  auto scheme_end = url.find(scheme_sep);
  size_t search_from = (scheme_end == std::string::npos) ? 0 : scheme_end + scheme_sep.length();
  auto path_start = url.find("/", search_from);
  if (path_start == std::string::npos) {
    host_ = url;
    endpoint_ = "/";
  } else {
    host_ = url.substr(0, path_start);
    endpoint_ = url.substr(path_start);
  }
}

std::string Notifier::getTimestampUTC() const {
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
    {"detected_at", a.detected_at.empty() ? getTimestampUTC() : a.detected_at},
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

bool Notifier::send(const Alert& alert) {
  if (host_.empty()) {
    std::cerr << "Notifier: webhook host is not ser\n";
    return false;
  }
  httplib::Client client(host_);
  client.set_connection_timeout(timeout_sec_, 0);
  client.set_read_timeout(timeout_sec_, 0);
  
  auto res = client.Post(endpoint_, toJson(alert).dump(), "application/json");
  if (res && res->status == 200) {
    logger.log(LOG_INFO, "Notifier: sent alert for " +  alert.file_path);
    return true;
  }
  std::cerr << "Notifier: failed to send alert for " << alert.file_path << std::endl;
  return false;
}

std::vector<Alert> Notifier::loadPending(const std::string& path) {
  std::vector<Alert> result;
  std::ifstream in(path);
  if (!in.is_open()) return result;
  
  json arr;
  try {
    in >> arr;
  } catch (...) {
    std::cerr << "Notifier: pending.json is corrupted\n";
    return result;
  }
  
  for (const auto& item : arr) result.push_back(fromJson(item));
  return result;
}

bool Notifier::savePending(const std::vector<Alert>& alerts, const std::string& path) {
  std::string tmp = path + ".tmp";
 // atomic write for save adding 
  {
    std::ofstream out(tmp);
    if (!out.is_open()) {
      std::cerr << "Notifier: cannot write to " << tmp << std::endl;
      return false;
    }
    json arr = json::array();
    for (const auto& a : alerts) arr.push_back(toJson(a));
    out << arr.dump(4);
  }
  
  std::error_code ec;
  std::filesystem::rename(tmp, path, ec);
  if (ec) {
    std::cerr << "Notifier: failed to rename tmp file " << tmp 
	      << ", error message " << ec.message() << std::endl;
    return false;
  }
  return true;
}

bool Notifier::addToPending(const Alert& alert, const std::string& path) {
  auto alerts = loadPending(path);
  for (auto& a : alerts) {
    if (a.file_path == alert.file_path){
      a = alert;
      return savePending(alerts, path);
    }
  }
  alerts.push_back(alert);
  return savePending(alerts, path);
}

bool Notifier::sendOrQueue(const Alert& alert, const std::string& pending_path) {
  if (send(alert, logger)) return true;
  return addToPending(alert, pending_path);
}

void Notifier::retryPending(const std::string& pending_path) {
  auto alerts = loadPending(pending_path);
  if (alerts.empty()) return;

  std::vector<Alert> still_pending;
  for (const auto& a : alerts) {
    if (!send(alert, logger)) {
      still_pending.push_back(alert);
    }
  }
  savePending(still_pending, pending_path);
}



