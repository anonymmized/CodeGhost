#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <string>
#include <iostream>
#include <cerrno>
#include <fstream>
#include <mutex>


namespace Logger {
  enum LogLevel { INFO = 0, WARN = 1, ERROR = 2 };
  struct FileInfo {
    std::string path;
    int line = 0;

  };

  class Logger {
  private:
    std::mutex mtx;
    std::string filename;
    std::ostream* out;
    std::ofstream file_out;
    LogLevel min_level = INFO;
    std::string fromLog(const LogLevel level, const std::string& message, const FileInfo& extraInfo);
    void writeToFileUnsafe(const std::string& text);
  public:
    explicit Logger(const std::string& filename = "", std::ostream& out = std::cout);
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    void setLevel(LogLevel level);
    void log(const LogLevel level, const std::string& message, const FileInfo& extraInfo = {});
  };
  Logger& getGlobalLogger();
  #define LOG_HERE Logger::FileInfo{__FILE__, __LINE__}

  #define LOG_INFO(msg) Logger::getGlobalLogger().log(Logger::INFO, msg, LOG_HERE)
  #define LOG_WARN(msg) Logger::getGlobalLogger().log(Logger::WARN, msg, LOG_HERE)
  #define LOG_ERROR(msg) Logger::getGlobalLogger().log(Logger::ERROR, msg, LOG_HERE)
};

#endif
