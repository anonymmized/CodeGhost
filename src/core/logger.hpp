#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <string>
#include <iostream>
#include <unordered_map>
#include <cerrno>
#include <fstream>
#include <mutex>


namespace Logger {
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
  public:
    enum LogLevel {INFO, WARN, ERROR};
    explicit Logger(const std::string& filename = "", std::ostream &out = std::cout);
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    std::string formLog(const LogLevel &level,
			const std::string &message,
			FileInfo extraInfo
			);
    void logMessage(const LogLevel &level,
		    const std::string &message,
		    FileInfo extraInfo
		    );
    void writeLogToFile(const std::string &message);
  };
  #define LOG_MESSAGE(logger, level, message, extraInfo) logger.logMessage(level, message, extraInfo)
  #define LOG_INFO(logger, message, extraInfo) logger.logMessage(Logger::INFO, message, extraInfo)
  #define LOG_WARN(logger, message, extraInfo) logger.logMessage(Logger::WARN, message, extraInfo)
  #define LOG_ERROR(logger, message, extraInfo) logger.logMessage(Logger::ERROR, message, extraInfo)

  #define LOG_MESSAGE_G(level, message, extraInfo) if(Logger::globalLogger) Logger::globalLogger->logMessage(level, message, extraInfo)
  #define LOG_INFO_G(message, extraInfo) if(Logger::globalLogger) Logger::globalLogger->logMessage(Logger::INFO, message, extraInfo)
  #define LOG_WARN_G(message, extraInfo) if(Logger::globalLogger) Logger::globalLogger->logMessage(Logger::WARN, message, extraInfo)
  #define LOG_ERROR_G(message, extraInfo) if(Logger::globalLogger) Logger::globalLogger->logMessage(Logger::ERROR, message, extraInfo)
  
  extern Logger* globalLogger;
};

#endif
