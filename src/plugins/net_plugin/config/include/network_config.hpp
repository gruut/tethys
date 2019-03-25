#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace gruut {
namespace net_plugin {

constexpr unsigned int DEPTH_B = 5;
constexpr unsigned int PARALLELISM_ALPHA = 3;
constexpr unsigned int KEYSIZE_BITS = 160;
constexpr unsigned int REDUNDANT_SAVE_COUNT = 3;
constexpr unsigned int KBUCKET_SIZE = 20;

constexpr unsigned int NODE_FAILED_COMMS_BEFORE_STALE = 2;
constexpr auto NODE_INACTIVE_TIME_BEFORE_QUESTIONABLE = std::chrono::minutes(15);

constexpr auto BUCKET_INACTIVE_TIME_BEFORE_REFRESH = std::chrono::seconds(1200);
constexpr auto REQUEST_TIMEOUT = std::chrono::seconds(2);

constexpr size_t GET_NODE_FROM_TRACKER_INTERVAL = 60; //minutes
constexpr size_t KEEP_BROADCAST_MSG_TIME = 180; //seconds
constexpr int MAX_NODE_INFO_FROM_TRACKER = 100;

//TODO : 임시적으로 사용. 변경될 것.
const std::string TRACKER_URL = "127.0.0.1:8080/nodeinfo";

//TODO : MERGER의 ID를 GA로 받아 올 수 있을때 사용치 않을것. (ID : TEST-MERGER-ID-1TEST-MERGER-ID-1 )
const std::string ID_BASE64 = "VEVTVC1NRVJHRVItSUQtMVRFU1QtTUVSR0VSLUlELTE=";
const std::string MY_ID =  "TEST-MERGER-ID-1TEST-MERGER-ID-1"; //32bytes
//TODO : LOCAL CHAIN ID 에 대해서 정해진 바가 없어 임시 사용 수정될 것.
const std::string LOCAL_CHAIN_ID = "T-LCHAINT-LCHAINT-LCHAINT-LCHAIN"; //32bytes
}  // namespace net
}  // namespace gruut
