#pragma once

#include "../../../lib/gruut-utils/src/lz4_compressor.hpp"
#include "../config/include/message.hpp"

namespace gruut {
namespace net_plugin {

using namespace std;

class MessagePacker {
public:
  static string packMessage(OutNetMsg &out_msg) {
    // TODO : serialized-body must be determined by serialization algo type
    string json_dump = out_msg.body.dump();
    string serialized_body = LZ4Compressor::compressData(json_dump);
    string header = makeHeader(serialized_body.size(), out_msg.type, SerializationAlgorithmType::LZ4);

    return (header + serialized_body);
  }

private:
  static string makeHeader(int serialized_json_size, MessageType msg_type, SerializationAlgorithmType serialization_algo_type) {
    MessageHeader msg_header;
    msg_header.identifier = IDENTIFIER;
    msg_header.version = VERSION;
    msg_header.message_type = msg_type;
    if (msg_type == MessageType::MSG_ACCEPT || msg_type == MessageType::MSG_REQ_SSIG)
      msg_header.mac_algo_type = MACAlgorithmType::HMAC;
    else
      msg_header.mac_algo_type = MACAlgorithmType::NONE;

    msg_header.serialization_algo_type = serialization_algo_type;
    msg_header.dummy = NOT_USED;

    int total_length = HEADER_LENGTH + serialized_json_size;

    for (int i = MSG_LENGTH_SIZE; i > 0; i--) {
      msg_header.total_length[i] |= total_length;
      total_length >>= 8;
    }
    msg_header.total_length[0] |= total_length;

    copy(begin(WORLD_ID), end(WORLD_ID), begin(msg_header.world_id));
    copy(begin(LOCAL_CHAIN_ID), end(LOCAL_CHAIN_ID), begin(msg_header.local_chain_id));
    copy(begin(MY_ID), end(MY_ID), begin(msg_header.sender_id));

    auto header_ptr = reinterpret_cast<uint8_t *>(&msg_header);
    auto serialized_header = string(header_ptr, header_ptr + sizeof(msg_header));

    return serialized_header;
  }
};

} // namespace net_plugin
} // namespace gruut