#pragma once

#include "../../../../lib/gruut-utils/src/lz4_compressor.hpp"
#include "../../../../lib/json/include/json.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "../config/include/message.hpp"
#include "../config/include/network_config.hpp"
#include "msg_schema.hpp"

#include <optional>

using namespace grpc;
using namespace std;

namespace gruut {
namespace net_plugin {

class MessageHandler {

public:
  optional<InNetMsg> unpackMsg(string &packed_msg, Status &return_rpc_status) {

    string raw_header(packed_msg.begin(), packed_msg.begin() + HEADER_LENGTH);
    auto msg_header = parseHeader(raw_header);
    if (!validateMsgFormat(msg_header)) {
      return_rpc_status = Status(StatusCode::INVALID_ARGUMENT, "Bad request (Invalid parameter)");
      return {};
    }

    auto body_size = convertU8ToU32BE(msg_header->total_length);
    string msg_raw_body(packed_msg.begin() + HEADER_LENGTH, packed_msg.begin() + HEADER_LENGTH + body_size);

    if (msg_header->mac_algo_type == MACAlgorithmType::HMAC) {
      vector<uint8_t> hmac(packed_msg.begin() + HEADER_LENGTH + body_size, packed_msg.end());

      // TODO : verify HMAC
    }

    auto json_body = getJson(msg_header->compression_algo_type, msg_raw_body);

    if (!JsonValidator::validateSchema(json_body, msg_header->message_type)) {
      return_rpc_status = Status(StatusCode::INVALID_ARGUMENT, "Bad request (Json schema error)");
      return {};
    }

    return_rpc_status = Status::OK;
    return InNetMsg{msg_header->message_type, json_body, msg_header->sender_id};
  }

  string packMsg(OutNetMsg &out_msg) {

    string json_dump = out_msg.body.dump();
    string compressed_body = LZ4Compressor::compressData(json_dump);
    string header = makeHeader(json_dump.size(), out_msg.type, CompressionAlgorithmType::LZ4);

    return (header + compressed_body);
  }

private:
  MessageHeader *parseHeader(string &raw_header) {
    auto msg_header = reinterpret_cast<MessageHeader *>(&raw_header);
    return msg_header;
  }

  string makeHeader(int compressed_json_size, MessageType msg_type, CompressionAlgorithmType compression_algo_type) {

    MessageHeader msg_header;
    msg_header.identifier = IDENTIFIER;
    msg_header.version = VERSION;
    msg_header.message_type = msg_type;
    if (msg_type == MessageType::MSG_ACCEPT || msg_type == MessageType::MSG_REQ_SSIG)
      msg_header.mac_algo_type = MACAlgorithmType::HMAC;
    else
      msg_header.mac_algo_type = MACAlgorithmType::NONE;

    msg_header.compression_algo_type = compression_algo_type;
    msg_header.dummy = NOT_USED;

    int total_length = HEADER_LENGTH + compressed_json_size;

    for (int i = MSG_LENGTH_SIZE; i > 0; i--) {
      msg_header.total_length[i] |= total_length;
      total_length >>= 8;
    }
    msg_header.total_length[0] |= total_length;

    std::copy(std::begin(LOCAL_CHAIN_ID), std::end(LOCAL_CHAIN_ID), std::begin(msg_header.local_chain_id));
    std::copy(std::begin(MY_ID), std::end(MY_ID), std::begin(msg_header.sender_id));

    auto header_ptr = reinterpret_cast<uint8_t *>(&msg_header);
    auto serialized_header = std::string(header_ptr, header_ptr + sizeof(msg_header));

    return serialized_header;
  }

  bool validateMsgFormat(MessageHeader *header) {
    bool check = header->identifier == IDENTIFIER;
    if (header->mac_algo_type == MACAlgorithmType::HMAC) {
      check &= (header->message_type == MessageType::MSG_SUCCESS || header->message_type == MessageType::MSG_SSIG);
    }
    return check;
  }

  int convertU8ToU32BE(array<uint8_t, MSG_LENGTH_SIZE> &len_bytes) {
    return static_cast<int>(len_bytes[0] << 24 | len_bytes[1] << 16 | len_bytes[2] << 8 | len_bytes[3]);
  }

  nlohmann::json getJson(CompressionAlgorithmType comperssion_type, string &raw_body) {
    nlohmann::json unpacked_body;
    if (!raw_body.empty()) {
      switch (comperssion_type) {
      case CompressionAlgorithmType::LZ4: {
        string origin_data = LZ4Compressor::decompressData(raw_body);
      }
      case CompressionAlgorithmType::NONE: {
        unpacked_body = json::parse(raw_body);
      }
      default:
        break;
      }
    }
    return unpacked_body;
  }
};
} // namespace net_plugin
} // namespace gruut