#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace gruut {
namespace net_plugin {

constexpr unsigned int PARALLELISM_ALPHA = 3;
constexpr auto GENERAL_SERVICE_TIMEOUT = std::chrono::milliseconds(100);

// TODO : LOCAL CHAIN ID / WORLD ID 에 대해서 정해진 바가 없어 임시 사용 수정될 것.
const std::string LOCAL_CHAIN_ID = "LCHAINID"; // 8bytes
const std::string WORLD_ID = "WORLD-ID";       // 8bytes
} // namespace net_plugin
} // namespace gruut
