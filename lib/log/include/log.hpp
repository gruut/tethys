#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Log {
  static auto console = spdlog::stdout_color_mt("console");
  static auto err_logger = spdlog::stderr_color_mt("stderr");
}

#define LOG_MESSAGE( LOG_LEVEL, FORMAT, ... ) \
   fc::log_message( FC_LOG_CONTEXT(LOG_LEVEL), FORMAT, fc::mutable_variant_object()__VA_ARGS__ )

#define INFO_LOG() \
  spdlog::info() \

#define WARNING_LOG()
#define ERROR_LOG()