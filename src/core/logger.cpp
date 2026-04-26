#include "logger.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstring>

namespace Logger {

  Logger::Logger(const std::string &filename, std::ostream& out) : filename(filename), out(out) {
    if (!this->filename.empty()) {
      file_out.open(this->filename, std::ios::app);
      if (!file_out) {
        std::cerr << "Logger: failed to open file: " << filename << "\n";
      }
    }
  }

  void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mtx);
    min_level = level;
  }

  std::string Logger::formLog(LogLevel level, const std::string& message, const FileInfo& extraInfo) {
    std::stringstream ss;
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm tm_buf{};
    localtime_r(&now_time, &tm_buf);
    ss << "[" << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "] ";

    switch (level) {
      case INFO: ss << "[INFO] "; break;
      case WARN: ss << "[WARN] "; break;
      case ERROR: ss << "[ERROR] "; break;
    }
    if (!extraInfo.path.empty()) {
      ss << "[" << extraInfo.path << ":" << extraInfo.line << "] ";
    }
    ss << message << "\n";
    return ss.str();
  }

  void Logger::writeToFileUnsafe(const std::string& text) {
    if (!file_out.is_open()) return;
    file_out << text;
    file_out.flush();
  }

  void Logger::log(LogLevel level, const std::string& message, const FileInfo& extraInfo) {
    if (level < min_level) return;
    std::lock_guard<std::mutex> lock(mtx);
    std::string text = formLog(level, message, extraInfo);
    out << text;
    writeToFileUnsafe(text);
  }

  Logger& getGlobalLogger() {
    static Logger instance("codeghost.log");
    return instance;
  }
}
