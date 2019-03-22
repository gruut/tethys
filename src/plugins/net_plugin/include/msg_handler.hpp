#pragma once

#include "../../../../lib/json/include/json.hpp"
#include "../config/include/message.hpp"
#include "../config/include/network_config.hpp"
#include "../../../gruut-utils/src/lz4_compressor.hpp"
#include "channel_interface.hpp"

using namespace grpc;
using namespace std;

namespace gruut {
  namespace net_plugin {

    class MessageHandler {

    public:
      InNetMsg unpackMsg(string &packed_msg, Status &return_rpc_status) {
        InNetMsg msg_entry;

        string raw_header(packed_msg.begin(), packed_msg.begin() + HEADER_LENGTH);
        auto msg_header = parseHeader(raw_header);
        if (!validateMsgFormat(msg_header)) {
          return_rpc_status = Status(StatusCode::INVALID_ARGUMENT, "Bad request (Invalid parameter)");
          return msg_entry;
        }

        auto body_size = convertU8ToU32BE(msg_header->total_length);
        string msg_raw_body(packed_msg.begin() + HEADER_LENGTH, packed_msg.begin() + HEADER_LENGTH + body_size);

        if (msg_header->mac_algo_type == MACAlgorithmType::HMAC) {
          vector<uint8_t> hmac(packed_msg.begin() + HEADER_LENGTH + body_size, packed_msg.end());

          //TODO : verify HMAC
        }

        auto json_body = getJson(msg_header->compression_algo_type, msg_raw_body);

        //TODO : validate json schema

        return_rpc_status = Status::OK;
        msg_entry.body = json_body;
        msg_entry.type = msg_header->message_type;
        msg_entry.sender_id = msg_header->sender_id;

        return msg_entry;
      }

    private:
      MessageHeader *parseHeader(string &raw_header) {
        auto msg_header = reinterpret_cast<MessageHeader *>(&raw_header);
        return msg_header;
      }

      bool validateMsgFormat(MessageHeader *header) {
        bool check = header->identifier == IDENTIFIER;
        if (header->mac_algo_type == MACAlgorithmType::HMAC) {
          check &= (header->message_type == MessageType::MSG_SUCCESS ||
                    header->message_type == MessageType::MSG_SSIG);
        }
        return check;
      }

      int convertU8ToU32BE(array<uint8_t, MSG_LENGTH_SIZE> &len_bytes) {
        return static_cast<int>(len_bytes[0] << 24 | len_bytes[1] << 16 |
                                len_bytes[2] << 8 | len_bytes[3]);
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
  }
}