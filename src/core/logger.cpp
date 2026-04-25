#include "logger.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Logger {
  Logger* globalLogger = nullptr;
  Logger::Logger(const std::string &filename, std::ostream &out) : filename(std::move(filename)), out(&out) {
    if (!this->filename.empty()) file_out.open(this->filename, std::ios::app);
  }
  void Logger::logMessage(const LogLevel &level, const std::string &message, FileInfo extraInfo) {
    std::lock_guard<std::mutex> lock(mtx);
    std::string formedText = formLog(level, message, extraInfo);
    if (out) {
      *out << formedText;
      out->flush();
    }
    writeLogToFile(formedText);
  };
  
  std::string Logger::formLog(const LogLevel &level, const std::string &message, FileInfo extraInfo){
    std::stringstream ss;
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    ss << "[" << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S") << "] ";
    switch (level) {
    case INFO:  ss << "[INFO] ";  break;
    case WARN:  ss << "[WARN] ";  break;
    case ERROR: ss << "[ERROR] "; break;
    }
    if (!extraInfo.path.empty()) ss << "[" << extraInfo.path << ":" << " ]";
    ss << message << "\n";
    return ss.str();
   };

  void Logger::writeLogToFile(const std::string &formedText){
    if (filename.empty()) return;
    if (!file_out.is_open() && !filename.empty()) {
      file_out.open(filename, std::ios::app);
    };
    if (file_out.is_open()) {
      file_out << formedText;
      file_out.flush();
    }
  };
}
