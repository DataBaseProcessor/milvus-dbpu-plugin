#include <dbpu/logger.h>
#include <fstream>

namespace dbpu {

Logger& Logger::instance() {
  static Logger l;
  return l;
}

void Logger::init(const std::string& path) {
  std::lock_guard<std::mutex> g(mu_);
  path_ = path;
}

void Logger::log(const std::string& line) {
  std::lock_guard<std::mutex> g(mu_);
  if (path_.empty()) return;
  std::ofstream ofs(path_, std::ios::app);
  if (!ofs) return;
  ofs << line << "\n";
}

} // namespace dbpu