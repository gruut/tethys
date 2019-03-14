#pragma once
#include <array>

namespace gruut {
namespace net_plugin {

enum class MACAlgorithmType : uint8_t {
  RSA = 0x00,
  ECDSA = 0x01,
  EdDSA = 0x02,
  Schnorr = 0x03,
  RSA_PKCS115 = 0x10,

  HMAC = 0xF1,
  NONE = 0xFF
};

enum class CompressionAlgorithmType : uint8_t {
  LZ4 = 0x04,
  MessagePack = 0x05,
  CBOR = 0x06,
  NONE = 0xFF
};

enum class MessageType : uint8_t {
  MSG_NULL = 0x00,
  MSG_UP = 0x30,
  MSG_PING = 0x31,
  MSG_REQ_BLOCK = 0x32,
  MSG_WELCOME = 0x35,
  MSG_REQ_STATUS = 0x33,
  MSG_RES_STATUS = 0x34,
  MSG_JOIN = 0x54,
  MSG_CHALLENGE = 0x55,
  MSG_RESPONSE_1 = 0x56,
  MSG_RESPONSE_2 = 0x57,
  MSG_SUCCESS = 0x58,
  MSG_ACCEPT = 0x59,
  MSG_LEAVE = 0x5B,

  MSG_JOIN_MERGER = 0x70,
  MSG_NETWORK_INFO = 0x71,
  MSG_CHAIN_INFO = 0x72,

  MSG_TX = 0xB1,
  MSG_REQ_SSIG = 0xB2,
  MSG_SSIG = 0xB3,
  MSG_BLOCK = 0xB4,
  MSG_HEADER = 0xB5,
  MSG_ERROR = 0xFF,
  MSG_REQ_CHECK = 0xC0,
  MSG_REQ_HEADER = 0xE0,
  MSG_RES_CHECK = 0xC1
};

using message_version_type = uint8_t;

constexpr int CHAIN_ID_TYPE_SIZE = 32;
using localchain_id_type = std::array<uint8_t, CHAIN_ID_TYPE_SIZE>;
using id_type = std::string;

constexpr int IDENTIFIER_LENGTH = 1;
constexpr int VERSION_LENGTH = 1;
constexpr int MSG_TYPE_LENGTH = 1;
constexpr int MAC_TYPE_LENGTH = 1;
constexpr int COMP_TYPE_LENGTH = 1;
constexpr int DUMMY_LENGTH = 1;

constexpr int MSG_LENGTH_SIZE = 4;
constexpr int SENDER_ID_TYPE_SIZE = 32;
constexpr int RESERVED_LENGTH = 6;

constexpr int HEADER_LENGTH =
	IDENTIFIER_LENGTH + VERSION_LENGTH + MSG_TYPE_LENGTH + MAC_TYPE_LENGTH
	+ COMP_TYPE_LENGTH + DUMMY_LENGTH + MSG_LENGTH_SIZE + CHAIN_ID_TYPE_SIZE
	+ SENDER_ID_TYPE_SIZE + RESERVED_LENGTH;

constexpr uint8_t G = 'G';

constexpr uint8_t VERSION = '1';
constexpr uint8_t NOT_USED = 0x00;
constexpr std::array<uint8_t, RESERVED_LENGTH> RESERVED{{0x00}};

struct MessageHeader {
  uint8_t identifier;
  message_version_type version;
  MessageType message_type;
  MACAlgorithmType mac_algo_type;
  CompressionAlgorithmType compression_algo_type;
  uint8_t dummy;
  std::array<uint8_t, MSG_LENGTH_SIZE> total_length;
  localchain_id_type local_chain_id;
  id_type sender_id;
  std::array<uint8_t, RESERVED_LENGTH> reserved_space;
};


} //namespace net
} //namespace gruut