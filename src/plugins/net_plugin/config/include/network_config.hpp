#pragma once

#include <cstddef>
#include <string>
#include <chrono>

namespace gruut {
namespace net_plugin {

constexpr unsigned int PARALLELISM_ALPHA = 3;
constexpr auto GENERAL_SERVICE_TIMEOUT = std::chrono::milliseconds(100);

// TODO : MERGER의 ID를 GA로 받아 올 수 있을때 사용치 않을것. (ID : TEST-MERGER-ID-1TEST-MERGER-ID-1 )
const std::string ID_BASE64 = "VEVTVC1NRVJHRVItSUQtMVRFU1QtTUVSR0VSLUlELTE=";
const std::string MY_ID = "TEST-MERGER-ID-1TEST-MERGER-ID-1"; // 32bytes
// TODO : LOCAL CHAIN ID / WORLD ID 에 대해서 정해진 바가 없어 임시 사용 수정될 것.
const std::string LOCAL_CHAIN_ID = "LCHAINID"; // 8bytes
const std::string WORLD_ID = "WORLD-ID"; // 8bytes
} // namespace net_plugin
} // namespace gruut
