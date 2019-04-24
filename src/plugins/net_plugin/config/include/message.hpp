#pragma once

#include "network_type.hpp"

#include <array>
#include <grpcpp/impl/codegen/status.h>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace gruut {
namespace net_plugin {

enum class MACAlgorithmType : uint8_t { HMAC = 0xF1, SHA256 = 0xF2, NONE = 0xFF };

enum class SerializationAlgorithmType : uint8_t { LZ4 = 0x04, MessagePack = 0x05, CBOR = 0x06, NONE = 0xFF };

enum class MessageType : uint8_t {
  MSG_PING = 0x31,
  MSG_REQ_BLOCK = 0x32,
  MSG_REQ_BONE = 0x36,

  MSG_JOIN = 0x54,
  MSG_CHALLENGE = 0x55,
  MSG_RESPONSE_1 = 0x56,
  MSG_RESPONSE_2 = 0x57,
  MSG_SUCCESS = 0x58,
  MSG_ACCEPT = 0x59,
  MSG_LEAVE = 0x5B,

  MSG_TX = 0xB1,
  MSG_REQ_SSIG = 0xB2,
  MSG_SSIG = 0xB3,
  MSG_BLOCK = 0xB4,
  MSG_BONE = 0xB8
};

using message_version_type = uint8_t;

constexpr int CHAIN_ID_TYPE_SIZE = 8;
constexpr int WORLD_ID_TYPE_SIZE = 8;
constexpr int SENDER_ID_TYPE_SIZE = 32;

using world_id_type = std::array<uint8_t, WORLD_ID_TYPE_SIZE>;
using localchain_id_type = std::array<uint8_t, CHAIN_ID_TYPE_SIZE>;
using sender_id_type = std::array<uint8_t, SENDER_ID_TYPE_SIZE>;

constexpr int IDENTIFIER_LENGTH = 1;
constexpr int VERSION_LENGTH = 1;
constexpr int MSG_TYPE_LENGTH = 1;
constexpr int MAC_TYPE_LENGTH = 1;
constexpr int SERIALIZATION_TYPE_LENGTH = 1;
constexpr int DUMMY_LENGTH = 1;
constexpr int MSG_LENGTH_SIZE = 4;

constexpr int HEADER_LENGTH = IDENTIFIER_LENGTH + VERSION_LENGTH + MSG_TYPE_LENGTH + MAC_TYPE_LENGTH + SERIALIZATION_TYPE_LENGTH +
                              DUMMY_LENGTH + MSG_LENGTH_SIZE + WORLD_ID_TYPE_SIZE + CHAIN_ID_TYPE_SIZE + SENDER_ID_TYPE_SIZE;

constexpr uint8_t IDENTIFIER = 'P';
constexpr uint8_t VERSION = 0x01;
constexpr uint8_t NOT_USED = 0x00;

struct InNetMsg {
  MessageType type;
  nlohmann::json body;
  b58_user_id_type sender_id;
};

struct OutNetMsg {
  MessageType type;
  nlohmann::json body;
  std::vector<b58_user_id_type> receivers;
};

struct MessageHeader {
  uint8_t identifier;
  message_version_type version;
  MessageType message_type;
  MACAlgorithmType mac_algo_type;
  SerializationAlgorithmType serialization_algo_type;
  uint8_t dummy;
  std::array<uint8_t, MSG_LENGTH_SIZE> total_length;
  world_id_type world_id;
  localchain_id_type local_chain_id;
  sender_id_type sender_id;
};

enum class ErrorMsgType : int {
  UNKNOWN = 0,
  MERGER_BOOTSTRAP = 11,
  ECDH_ILLEGAL_ACCESS = 21,
  ECDH_MAX_SIGNER_POOL = 22,
  ECDH_TIMEOUT = 23,
  ECDH_INVALID_SIG = 24,
  ECDH_INVALID_PK = 25,
  TIME_SYNC = 61,
  BSYNC_NO_BLOCK = 88,
  NO_SUCH_BLOCK = 89
};

enum class MsgEntryType {
  TIMESTAMP,
  BASE58_256,
  BASE64,
  BASE64_256,
  BASE64_SIG,
  DECIMAL,
  ALPHA_64,
  PEM,
  PK,
  PEM_PK,
  HEX_256,
  CONTRACT_ID,
  BOOL,
  NONE
};

enum class MsgEntryLength : int { NOT_LIMITED = 0, BASE64_256 = 44, BASE58_256 = 44, ALPHA_64 = 8, HEX_256 = 64 };

} // namespace net_plugin
} // namespace gruut