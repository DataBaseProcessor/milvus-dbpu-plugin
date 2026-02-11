#pragma once
#include <string>
#include <mutex>

namespace dbpu {

class Logger {
public:
  static Logger& instance();
  void init(const std::string& path);
  void log(const std::string& line);

private:
  Logger() = default;
  std::mutex mu_;
  std::string path_;
};

} // namespace dbpu