#pragma once

#include "../../../lib/gruut-utils/src/hmac.hpp"
#include "../../../lib/gruut-utils/src/lz4_compressor.hpp"
#include "../config/include/message.hpp"

namespace gruut {
namespace net_plugin {

using namespace std;

class MessagePacker {
public:
  template <MACAlgorithmType T, typename = enable_if_t<T == MACAlgorithmType::NONE>>
  static string packMessage(OutNetMsg &out_msg) {
    return pack(out_msg);
  }

  template <MACAlgorithmType T, typename = enable_if_t<T == MACAlgorithmType::HMAC>>
  static string packMessage(OutNetMsg &out_msg, vector<uint8_t> &hmac_key) {
    auto packed_msg = pack(out_msg);
    auto hmac = Hmac::generateHMAC(packed_msg, hmac_key);
    return packed_msg + string(hmac.begin(), hmac.end());
  }

private:
  static string pack(OutNetMsg &out_msg) {
    // TODO : serialized-body must be determined by serialization algo type
    string json_dump = out_msg.body.dump();
    string serialized_body = LZ4Compressor::compressData(json_dump);
    string header = makeHeader(serialized_body.size(), out_msg.type, SerializationAlgorithmType::LZ4);

    return (header + serialized_body);
  }

  static string makeHeader(int serialized_json_size, MessageType msg_type, SerializationAlgorithmType serialization_algo_type) {
    string serialized_header;

    serialized_header.push_back(IDENTIFIER);
    serialized_header.push_back(VERSION);
    serialized_header.push_back((uint8_t)msg_type);
    if (msg_type == MessageType::MSG_ACCEPT || msg_type == MessageType::MSG_REQ_SSIG)
      serialized_header.push_back((uint8_t)MACAlgorithmType::HMAC);
    else
      serialized_header.push_back((uint8_t)MACAlgorithmType::NONE);

    serialized_header.push_back((uint8_t)serialization_algo_type);
    serialized_header.push_back(NOT_USED);

    auto total_length = TypeConverter::integerToBytes(HEADER_LENGTH + serialized_json_size);
    serialized_header.insert(serialized_header.end(), begin(total_length), end(total_length));
    serialized_header.insert(serialized_header.end(), begin(WORLD_ID), end(WORLD_ID));
    serialized_header.insert(serialized_header.end(), begin(LOCAL_CHAIN_ID), end(LOCAL_CHAIN_ID));
    serialized_header.insert(serialized_header.end(), begin(MY_ID), end(MY_ID));
    return serialized_header;
  }
};

} // namespace net_plugin
} // namespace gruut