#include "include/log.hpp"

namespace logger {
Logger &Logger::get() {
  static Logger logger;

  return logger;
}
} // namespace logger
