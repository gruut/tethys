#pragma once
#include "../../../../lib/json/include/json.hpp"
#include "../../../../lib/tethys-utils/src/lz4_compressor.hpp"
#include "../../../../lib/tethys-utils/src/type_converter.hpp"
#include "../config/include/message.hpp"
#include "msg_schema.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace tethys {
namespace net_plugin {

using namespace std;
using namespace grpc;

class MessageParser {
public:
  optional<InNetMsg> parseMessage(string_view packed_msg, string &return_err_info) {
    const string raw_header(packed_msg.begin(), packed_msg.begin() + HEADER_LENGTH);
    auto msg_header = parseHeader(raw_header);
    if (!validateMsgHdrFormat(msg_header)) {
      return_err_info = "Bad request (Invalid message header format)";
      return {};
    }
    auto body_size = convertU8ToU32BE(msg_header.total_length) - HEADER_LENGTH;
    string msg_raw_body(packed_msg.begin() + HEADER_LENGTH, packed_msg.begin() + HEADER_LENGTH + body_size);
    auto json_body = getJson(msg_header.serialization_algo_type, msg_raw_body);

    if (!JsonValidator::validateSchema(json_body, msg_header.message_type)) {
      return_err_info = "Bad request (Json schema error)";
      return {};
    }

    InNetMsg in_msg;
    in_msg.type = msg_header.message_type;
    in_msg.body = json_body;
    in_msg.sender_id = TypeConverter::encodeBase<58>(msg_header.sender_id);

    return in_msg;
  }

private:
  MessageHeader parseHeader(string_view raw_header) {
    MessageHeader msg_header;

    int offset = 0;
    msg_header.identifier = raw_header[offset++];
    msg_header.version = raw_header[offset++];
    msg_header.message_type = (MessageType)raw_header[offset++];
    msg_header.mac_algo_type = (MACAlgorithmType)raw_header[offset++];
    msg_header.serialization_algo_type = (SerializationAlgorithmType)raw_header[offset++];
    msg_header.dummy = raw_header[offset++];

    copy(raw_header.begin() + offset, raw_header.begin() + offset + MSG_LENGTH_SIZE, msg_header.total_length.begin());
    offset += MSG_LENGTH_SIZE;
    copy(raw_header.begin() + offset, raw_header.begin() + offset + WORLD_ID_TYPE_SIZE, msg_header.world_id.begin());
    offset += WORLD_ID_TYPE_SIZE;
    copy(raw_header.begin() + offset, raw_header.begin() + offset + CHAIN_ID_TYPE_SIZE, msg_header.local_chain_id.begin());
    offset += CHAIN_ID_TYPE_SIZE;
    copy(raw_header.begin() + offset, raw_header.begin() + offset + SENDER_ID_TYPE_SIZE, msg_header.sender_id.begin());
    return msg_header;
  }

  bool validateMsgHdrFormat(MessageHeader &header) {
    bool check = header.identifier == IDENTIFIER;
    if (header.mac_algo_type == MACAlgorithmType::HMAC) {
      check &= (header.message_type == MessageType::MSG_SUCCESS || header.message_type == MessageType::MSG_SSIG ||
                header.message_type == MessageType::MSG_REQ_TX_CHECK || header.message_type == MessageType::MSG_QUERY ||
                header.message_type == MessageType::MSG_TX);
    }
    return check;
  }

  int convertU8ToU32BE(array<uint8_t, MSG_LENGTH_SIZE> &len_bytes) {
    return static_cast<int>(len_bytes[0] << 24 | len_bytes[1] << 16 | len_bytes[2] << 8 | len_bytes[3]);
  }

  nlohmann::json getJson(SerializationAlgorithmType comperssion_type, string &raw_body) {
    nlohmann::json unpacked_body;
    if (!raw_body.empty()) {
      switch (comperssion_type) {
      case SerializationAlgorithmType::LZ4: {
        string origin_data = LZ4Compressor::decompressData(raw_body);
        unpacked_body = json::parse(origin_data);
        break;
      }
      case SerializationAlgorithmType::NONE: {
        unpacked_body = json::parse(raw_body);
        break;
      }
      case SerializationAlgorithmType::CBOR: {
        unpacked_body = nlohmann::json::from_cbor(raw_body, true, false);
      }
      default:
        break;
      }
    }
    return unpacked_body;
  }
};
} // namespace net_plugin
}