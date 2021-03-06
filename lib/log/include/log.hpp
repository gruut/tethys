#pragma once

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>

namespace logger {
class Logger {
public:
  ~Logger() {
    spdlog::drop_all();
  };

  Logger(const Logger &logger) = delete;

  Logger(const Logger &&logger) = delete;

  Logger &operator=(const Logger &) = delete;

  static Logger &get();

  template <typename... Arg>
  static void error(const Arg &... args) {
    get().error_logger->error(args...);
  }

  template <typename... Arg>
  static void info(const Arg &... args) {
    get().info_logger->info(args...);
  }

private:
  Logger() : info_logger{spdlog::stdout_color_mt("INFO")}, error_logger{spdlog::stdout_color_mt("ERROR")} {
    info_logger->set_pattern("%^[%H:%M:%S] [%l] [thread %t] --- %v%$");
    error_logger->set_pattern("%^[%H:%M:%S] [%l] [thread %t] --- %v%$");

    auto info_sink = dynamic_cast<spdlog::sinks::stdout_color_sink_mt *>(info_logger->sinks().back().get());
    info_sink->set_color(spdlog::level::info, info_sink->white);

    auto error_sink = dynamic_cast<spdlog::sinks::stdout_color_sink_mt *>(error_logger->sinks().back().get());
    error_sink->set_color(spdlog::level::err, error_sink->on_red);
  }

  std::shared_ptr<spdlog::logger> info_logger;
  std::shared_ptr<spdlog::logger> error_logger;
};

#define ERROR(FORMAT, ...) Logger::error(FORMAT, ##__VA_ARGS__)
#define INFO(FORMAT, ...) Logger::info(FORMAT, ##__VA_ARGS__)
} // namespace logger
