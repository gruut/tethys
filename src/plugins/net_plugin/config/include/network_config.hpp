#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace tethys {
namespace net_plugin {

constexpr unsigned int PARALLELISM_ALPHA = 3;
constexpr auto GENERAL_SERVICE_TIMEOUT = std::chrono::milliseconds(100);

} // namespace net_plugin
} // namespace tethys
